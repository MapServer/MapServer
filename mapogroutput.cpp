/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  OGR Output (for WFS)
 * Author:   Frank Warmerdam (warmerdam@pobox.com)
 *
 **********************************************************************
 * Copyright (c) 2010, Frank Warmerdam <warmerdam@pobox.com>
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
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#include <assert.h>
#include "mapserver.h"
#include "mapproject.h"
#include "mapthread.h"
#include "mapows.h"

#include "ogr_api.h"
#include "ogr_srs_api.h"
#include "cpl_conv.h"
#include "cpl_vsi.h"
#include "cpl_string.h"

#include <string>

/************************************************************************/
/*                   msInitDefaultOGROutputFormat()                     */
/************************************************************************/

int msInitDefaultOGROutputFormat( outputFormatObj *format )

{
  OGRSFDriverH   hDriver;

  msOGRInitialize();

  /* -------------------------------------------------------------------- */
  /*      check that this driver exists.  Note visiting drivers should    */
  /*      be pretty threadsafe so don't bother acquiring the GDAL         */
  /*      lock.                                                           */
  /* -------------------------------------------------------------------- */
  hDriver = OGRGetDriverByName( format->driver+4 );
  if( hDriver == NULL ) {
    msSetError( MS_MISCERR, "No OGR driver named `%s' available.",
                "msInitDefaultOGROutputFormat()", format->driver+4 );
    return MS_FAILURE;
  }

  if( !OGR_Dr_TestCapability( hDriver, ODrCCreateDataSource ) ) {
    msSetError( MS_MISCERR, "OGR `%s' driver does not support output.",
                "msInitDefaultOGROutputFormat()", format->driver+4 );
    return MS_FAILURE;
  }

  /* -------------------------------------------------------------------- */
  /*      Initialize the object.                                          */
  /* -------------------------------------------------------------------- */
  format->imagemode = MS_IMAGEMODE_FEATURE;
  format->renderer = MS_RENDER_WITH_OGR;

  /* perhaps we should eventually hardcode mimetypes and extensions
     for some formats? */

  return MS_SUCCESS;
}

/************************************************************************/
/*                        msCSLConcatenate()                            */
/************************************************************************/

static char** msCSLConcatenate( char** papszResult, char** papszToBeAdded )
{
    char** papszIter = papszToBeAdded;
    while( papszIter && *papszIter )
    {
        papszResult = CSLAddString(papszResult, *papszIter);
        papszIter ++;
    }
    return papszResult;
}

/************************************************************************/
/*                       msOGRRecursiveFileList()                       */
/*                                                                      */
/*      Collect a list of all files under the named directory,          */
/*      including those in subdirectories.                              */
/************************************************************************/

char **msOGRRecursiveFileList( const char *path )
{
  char **file_list;
  char **result_list = NULL;
  int i, count, change;

  file_list = CPLReadDir( path );
  count = CSLCount(file_list);

  /* -------------------------------------------------------------------- */
  /*      Sort the file list so we always get them back in the same       */
  /*      order - it makes autotests more stable.                         */
  /* -------------------------------------------------------------------- */
  do {
    change = 0;
    for( i = 0; i < count-1; i++ ) {
      if( strcasecmp(file_list[i],file_list[i+1]) > 0 ) {
        char *temp = file_list[i];
        file_list[i] = file_list[i+1];
        file_list[i+1] = temp;
        change = 1;
      }
    }
  } while( change );

  /* -------------------------------------------------------------------- */
  /*      collect names we want and process subdirectories.               */
  /* -------------------------------------------------------------------- */
  for( i = 0; i < count; i++ ) {
    char full_filename[MS_MAXPATHLEN];
    VSIStatBufL  sStatBuf;

    if( EQUAL(file_list[i],".") || EQUAL(file_list[i],"..") )
      continue;

    strlcpy( full_filename,
             CPLFormFilename( path, file_list[i], NULL ),
             sizeof(full_filename) );

    if( VSIStatL( full_filename, &sStatBuf ) != 0 )
        continue;

    if( VSI_ISREG( sStatBuf.st_mode ) ) {
      result_list = CSLAddString( result_list, full_filename );
    } else if( VSI_ISDIR( sStatBuf.st_mode ) ) {
      char **subfiles = msOGRRecursiveFileList( full_filename );

      result_list = msCSLConcatenate( result_list, subfiles );

      CSLDestroy( subfiles );
    }
  }

  CSLDestroy( file_list );

  return result_list;
}

/************************************************************************/
/*                           msOGRCleanupDS()                           */
/************************************************************************/
static void msOGRCleanupDS( const char *datasource_name )

{
  char **file_list;
  char path[MS_MAXPATHLEN];
  int i;
  VSIStatBufL sStatBuf;
  
  if( VSIStatL( datasource_name, &sStatBuf ) != 0 )
    return;
  if( VSI_ISDIR( sStatBuf.st_mode ) )
    strlcpy( path, datasource_name, sizeof(path) );
  else
    strlcpy( path, CPLGetPath( datasource_name ), sizeof(path) );

  file_list = CPLReadDir( path );

  for( i = 0; file_list != NULL && file_list[i] != NULL; i++ ) {
    char full_filename[MS_MAXPATHLEN];
    VSIStatBufL  sStatBuf;

    if( EQUAL(file_list[i],".") || EQUAL(file_list[i],"..") )
      continue;

    strlcpy( full_filename,
             CPLFormFilename( path, file_list[i], NULL ),
             sizeof(full_filename) );

    if( VSIStatL( full_filename, &sStatBuf ) != 0 )
      continue;

    if( VSI_ISREG( sStatBuf.st_mode ) ) {
      VSIUnlink( full_filename );
    } else if( VSI_ISDIR( sStatBuf.st_mode ) ) {
      msOGRCleanupDS( full_filename );
    }
  }

  CSLDestroy( file_list );

  VSIRmdir( path );
}

/************************************************************************/
/*                          msOGRSetPoints()                            */
/************************************************************************/

static void msOGRSetPoints( OGRGeometryH hGeom, lineObj *line, int bWant2DOutput)
{
    int i;
    if( bWant2DOutput )
    {
      for( i = 0; i < line->numpoints; i++ ) {
        OGR_G_SetPoint_2D( hGeom, i,
                           line->point[i].x,
                           line->point[i].y );
      }
    }
    else {
      for( i = 0; i < line->numpoints; i++ ) {
        OGR_G_SetPoint( hGeom, i,
                        line->point[i].x,
                        line->point[i].y,
#ifdef USE_POINT_Z_M
                        line->point[i].z
#else
                        0.0
#endif
                      );
      }
    }
}

/************************************************************************/
/*                          msOGRWriteShape()                           */
/************************************************************************/

static int msOGRWriteShape( layerObj *map_layer, OGRLayerH hOGRLayer,
                            shapeObj *shape, gmlItemListObj *item_list,
                            int nFirstOGRFieldIndex, const char *pszFeatureid )

{
  OGRGeometryH hGeom = NULL;
  OGRFeatureH hFeat;
  OGRErr eErr;
  int i, out_field;
  OGRwkbGeometryType eLayerGType, eFlattenLayerGType;
  OGRFeatureDefnH hLayerDefn;
  int bWant2DOutput;

  hLayerDefn = OGR_L_GetLayerDefn( hOGRLayer );
  eLayerGType = OGR_FD_GetGeomType(hLayerDefn);
  eFlattenLayerGType = wkbFlatten(eLayerGType);
  bWant2DOutput = (eLayerGType == eFlattenLayerGType);

  /* -------------------------------------------------------------------- */
  /*      Transform point geometry.                                       */
  /* -------------------------------------------------------------------- */
  if( shape->type == MS_SHAPE_POINT ) {
    OGRGeometryH hMP = NULL;
    int j;

    if( shape->numlines < 1 ) {
      msSetError(MS_MISCERR,
                 "Failed on odd point geometry.",
                 "msOGRWriteShape()");
      return MS_FAILURE;
    }
    
    if( shape->numlines == 1 && shape->line[0].numpoints > 1 )
    {
      hGeom = OGR_G_CreateGeometry( wkbMultiPoint );
      for( j = 0; j < shape->line[0].numpoints; j++ ) {
        OGRGeometryH hPoint = OGR_G_CreateGeometry( wkbPoint );
        if( bWant2DOutput ) {
            OGR_G_SetPoint_2D( hPoint, 0,
                            shape->line[0].point[j].x,
                            shape->line[0].point[j].y );
        }
        else {
            OGR_G_SetPoint( hPoint, 0,
                            shape->line[0].point[j].x,
                            shape->line[0].point[j].y,
#ifdef USE_POINT_Z_M
                            shape->line[0].point[j].z
#else
                            0.0
#endif
                            );
        }

        OGR_G_AddGeometryDirectly( hGeom, hPoint );
      }
    }
    else
    {
      if( shape->numlines > 1 )
        hMP = OGR_G_CreateGeometry( wkbMultiPoint );

      for( j = 0; j < shape->numlines; j++ ) {
        if( shape->line[j].numpoints != 1 ) {
          msSetError(MS_MISCERR,
                    "Failed on odd point geometry.",
                    "msOGRWriteShape()");
          return MS_FAILURE;
        }

        hGeom = OGR_G_CreateGeometry( wkbPoint );
        if( bWant2DOutput ) {
            OGR_G_SetPoint_2D( hGeom, 0,
                            shape->line[j].point[0].x,
                            shape->line[j].point[0].y );
        }
        else {
            OGR_G_SetPoint( hGeom, 0,
                            shape->line[j].point[0].x,
                            shape->line[j].point[0].y,
#ifdef USE_POINT_Z_M
                            shape->line[j].point[0].z
#else
                            0.0
#endif
                            );
        }

        if( hMP != NULL ) {
            OGR_G_AddGeometryDirectly( hMP, hGeom );
        }
      }
      
      if( hMP != NULL )
        hGeom = hMP;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Transform line geometry.                                        */
  /* -------------------------------------------------------------------- */
  else if(  shape->type == MS_SHAPE_LINE ) {
    OGRGeometryH hML = NULL;
    int j;

    if( shape->numlines < 1 || shape->line[0].numpoints < 2 ) {
      msSetError(MS_MISCERR,
                 "Failed on odd line geometry.",
                 "msOGRWriteShape()");
      return MS_FAILURE;
    }

    if( shape->numlines > 1 )
      hML = OGR_G_CreateGeometry( wkbMultiLineString );

    for( j = 0; j < shape->numlines; j++ ) {
      hGeom = OGR_G_CreateGeometry( wkbLineString );

      msOGRSetPoints( hGeom, &(shape->line[j]), bWant2DOutput);

      if( hML != NULL ) {
        OGR_G_AddGeometryDirectly( hML, hGeom );
        hGeom = hML;
      }
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Transform polygon geometry.                                     */
  /* -------------------------------------------------------------------- */
  else if( shape->type == MS_SHAPE_POLYGON ) {
    int iRing, iOuter;
    int *outer_flags;
    OGRGeometryH hMP;

    if( shape->numlines < 1 ) {
      msSetError(MS_MISCERR,
                 "Failed on odd polygon geometry.",
                 "msOGRWriteShape()");
      return MS_FAILURE;
    }

    outer_flags = msGetOuterList( shape );
    hMP = OGR_G_CreateGeometry( wkbMultiPolygon );

    for( iOuter = 0; iOuter < shape->numlines; iOuter++ ) {
      int *inner_flags;
      OGRGeometryH hRing;

      if( !outer_flags[iOuter] )
        continue;

      hGeom = OGR_G_CreateGeometry( wkbPolygon );

      /* handle outer ring */

      hRing = OGR_G_CreateGeometry( wkbLinearRing );

      msOGRSetPoints( hRing, &(shape->line[iOuter]), bWant2DOutput);

      OGR_G_AddGeometryDirectly( hGeom, hRing );


      /* handle inner rings (holes) */
      inner_flags = msGetInnerList( shape, iOuter, outer_flags );

      for( iRing = 0; iRing < shape->numlines; iRing++ ) {
        if( !inner_flags[iRing] )
          continue;

        hRing = OGR_G_CreateGeometry( wkbLinearRing );

        msOGRSetPoints( hRing, &(shape->line[iRing]), bWant2DOutput);

        OGR_G_AddGeometryDirectly( hGeom, hRing );
      }

      free(inner_flags);

      OGR_G_AddGeometryDirectly( hMP, hGeom );
    }

    free(outer_flags);

    if( OGR_G_GetGeometryCount( hMP ) == 1 ) {
      hGeom = OGR_G_Clone( OGR_G_GetGeometryRef( hMP, 0 ) );
      OGR_G_DestroyGeometry( hMP );
    } else {
      hGeom = hMP;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Consider trying to force the geometry to a new type if it       */
  /*      doesn't match the layer.                                        */
  /* -------------------------------------------------------------------- */
  if( hGeom != NULL ) {
    OGRwkbGeometryType eFlattenFeatureGType =
        wkbFlatten(OGR_G_GetGeometryType( hGeom ));

    if( eFlattenFeatureGType != eFlattenLayerGType ) {
      if( eFlattenLayerGType == wkbPolygon )
        hGeom = OGR_G_ForceToPolygon( hGeom );

      else if( eFlattenLayerGType == wkbMultiPolygon )
        hGeom = OGR_G_ForceToMultiPolygon( hGeom );

      else if( eFlattenLayerGType == wkbMultiPoint )
        hGeom = OGR_G_ForceToMultiPoint( hGeom );

      else if( eFlattenLayerGType == wkbMultiLineString )
        hGeom = OGR_G_ForceToMultiLineString( hGeom );
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Consider flattening the geometry to 2D if we want 2D            */
  /*      output.                                                         */
  /*      Note: this shouldn't be called in recent OGR versions where     */
  /*      OGR_G_SetPoint_2D is properly honoured.                         */
  /* -------------------------------------------------------------------- */

  if( bWant2DOutput && hGeom != NULL ) {
    OGRwkbGeometryType eFeatureGType = OGR_G_GetGeometryType( hGeom );
    if( eFeatureGType != wkbFlatten(eFeatureGType) )
      OGR_G_FlattenTo2D( hGeom );
  }

  /* -------------------------------------------------------------------- */
  /*      Create the feature, and attach the geometry.                    */
  /* -------------------------------------------------------------------- */
  hFeat = OGR_F_Create( hLayerDefn );

  OGR_F_SetGeometryDirectly( hFeat, hGeom );

  /* -------------------------------------------------------------------- */
  /*      Set attributes.                                                 */
  /* -------------------------------------------------------------------- */
  out_field = nFirstOGRFieldIndex;
  for( i = 0; i < item_list->numitems; i++ ) {
    gmlItemObj *item = item_list->items + i;

    if(pszFeatureid && !strcmp(pszFeatureid, item->name)) {
      char *endptr;
      long feature_id = strtol(shape->values[i],&endptr,10);
      if(endptr && *endptr==0) {
        /* only set the featureid if it is numeric */
        OGR_F_SetFID(hFeat, feature_id);
      }
    }

    if( !item->visible )
      continue;

    /* Avoid setting empty strings for numeric fields, so that OGR */
    /* doesn't take them as 0. (#4633) */
    if( shape->values[i][0] == '\0' ) {
      OGRFieldDefnH hFieldDefn = OGR_FD_GetFieldDefn(hLayerDefn, out_field);
      OGRFieldType eFieldType = OGR_Fld_GetType(hFieldDefn);
      if( eFieldType == OFTInteger || eFieldType == OFTReal
          || eFieldType == OFTInteger64
          )
      {
        out_field++;
        continue;
      }
    }

    OGR_F_SetFieldString( hFeat, out_field++, shape->values[i] );
  }

  /* -------------------------------------------------------------------- */
  /*      Write out and cleanup.                                          */
  /* -------------------------------------------------------------------- */
  eErr = OGR_L_CreateFeature( hOGRLayer, hFeat );

  OGR_F_Destroy( hFeat );

  if( eErr != OGRERR_NONE ) {
    msSetError( MS_OGRERR,
                "Attempt to write feature failed (code=%d):\n%s",
                "msOGRWriteShape()",
                (int) eErr,
                CPLGetLastErrorMsg() );
  }

  if( eErr == OGRERR_NONE )
    return MS_SUCCESS;
  else
    return MS_FAILURE;
}

/************************************************************************/
/*                        msOGRStdoutWriteFunction()                    */
/************************************************************************/

/* Used by /vsistdout/ */
static size_t msOGRStdoutWriteFunction(const void* ptr, size_t size, size_t nmemb, FILE* stream)
{
    msIOContext *ioctx = (msIOContext*) stream;
    return msIO_contextWrite(ioctx, ptr, size * nmemb ) / size;
}

/************************************************************************/
/*                      msOGROutputGetAdditonalFiles()                  */
/*                                                                      */
/*  Collect additional files specified in                               */
/*  wfs/ows_additional_files_in_output of WEB.METADATA and LAYER.METADATA */
/************************************************************************/

/* Result to be freed with CSLDestroy() */
static char** msOGROutputGetAdditonalFiles( mapObj *map )
{
    int i;
    hashTableObj* hSetAdditionalFiles;
    char** papszFiles = NULL;

    hSetAdditionalFiles = msCreateHashTable();

    for( i = -1; i < map->numlayers; i++ )
    {
        const char* value;
        if( i < 0 )
        {
            value = msOWSLookupMetadata(&(map->web.metadata), "FO", "additional_files_in_output");
        }
        else
        {
            layerObj *layer = GET_LAYER(map, i);
            if( !layer->resultcache || layer->resultcache->numresults == 0 )
                continue;
            value = msOWSLookupMetadata(&(layer->metadata), "FO", "additional_files_in_output");
        }

        if( value != NULL )
        {
            char** papszList = CSLTokenizeString2( value, ",", CSLT_HONOURSTRINGS );
            char** papszListIter = papszList;
            while( papszListIter && *papszListIter )
            {
                const char* file = *papszListIter;
                VSIStatBufL sStat;

                if( strncmp(file, "http://", strlen("http://")) == 0 ||
                    strncmp(file, "https://", strlen("https://")) == 0 )
                {
                    /* Remote file ? We will use /vsicurl_streaming/ to read it */
                    if( msLookupHashTable(hSetAdditionalFiles, file) == NULL )
                    {
                        msInsertHashTable(hSetAdditionalFiles, file, "YES");
                        papszFiles = CSLAddString(papszFiles, CPLSPrintf("/vsicurl_streaming/%s", file));
                    }
                }
                else
                {
                    int nLen = (int)strlen(file);
                    char filename[MS_MAXPATHLEN];

                    if( CPLIsFilenameRelative(file) )
                    {
                        if( !map->shapepath )
                            msTryBuildPath(filename, map->mappath, file);
                        else
                            msTryBuildPath3(filename, map->mappath, map->shapepath, file);
                    }
                    else
                        strlcpy(filename, file, MS_MAXPATHLEN);

                    if( nLen > 2 && (
                            strcmp(file + nLen - 1, "/") == 0 ||
                            strcmp(file + nLen - 2, "/*") == 0 ) )
                    {
                        *strrchr(filename, '/') = '\0';
                    }
                    else if( nLen > 2 && (
                            strcmp(file + nLen - 1, "\\") == 0 ||
                            strcmp(file + nLen - 2, "\\*") == 0 ) )
                    {
                        *strrchr(filename, '\\') = '\0';
                    }

                    if( msLookupHashTable(hSetAdditionalFiles, filename) == NULL )
                    {
                        msInsertHashTable(hSetAdditionalFiles, filename, "YES");
                        if( VSIStatL( filename, &sStat ) == 0 )
                        {
                            if( VSI_ISDIR( sStat.st_mode ) )
                            {
                                char** papszDirContent = msOGRRecursiveFileList(filename);
                                papszFiles = msCSLConcatenate(papszFiles, papszDirContent);
                                CSLDestroy(papszDirContent);
                            }
                            else
                            {
                                papszFiles = CSLAddString(papszFiles, filename);
                            }
                        }
                        else
                        {
                            msDebug("File %s does not exist.\n", filename);
                        }
                    }
                }

                papszListIter ++;
            }
            CSLDestroy(papszList);
        }
    }
    
    msFreeHashTable(hSetAdditionalFiles);
    
    return papszFiles;
}

/************************************************************************/
/*                        msOGRWriteFromQuery()                         */
/************************************************************************/

int msOGRWriteFromQuery( mapObj *map, outputFormatObj *format, int sendheaders )

{
  /* -------------------------------------------------------------------- */
  /*      Variable declarations.                                          */
  /* -------------------------------------------------------------------- */
  OGRSFDriverH hDriver;
  OGRDataSourceH hDS;
  const char *storage;
  const char *fo_filename;
  const char *form;
  char datasource_name[MS_MAXPATHLEN];
  char base_dir[MS_MAXPATHLEN];
  char *request_dir = NULL;
  char **ds_options = NULL;
  char **layer_options = NULL;
  char **file_list = NULL;
  int iLayer, i;
  int bDataSourceNameIsRequestDir = FALSE;
  int bUseFeatureId = MS_FALSE;
  const char* pszMatchingFeatures;
  int nMatchingFeatures = -1;
  const char* pszFormatName = format->driver+4;
  
  pszMatchingFeatures = msGetOutputFormatOption(format, "_matching_features_", "");
  if( pszMatchingFeatures[0] != '\0' )
      nMatchingFeatures = atoi(pszMatchingFeatures);

  /* -------------------------------------------------------------------- */
  /*      Fetch the output format driver.                                 */
  /* -------------------------------------------------------------------- */
  msOGRInitialize();

  hDriver = OGRGetDriverByName( pszFormatName );
  if( hDriver == NULL ) {
    msSetError( MS_MISCERR, "No OGR driver named `%s' available.",
                "msOGRWriteFromQuery()", pszFormatName );
    return MS_FAILURE;
  }

  /* -------------------------------------------------------------------- */
  /*      Capture datasource and layer creation options.                  */
  /* -------------------------------------------------------------------- */
  for( i=0; i < format->numformatoptions; i++ ) {
    if( strncasecmp(format->formatoptions[i],"LCO:",4) == 0 )
      layer_options = CSLAddString( layer_options,
                                    format->formatoptions[i] + 4 );
    if( strncasecmp(format->formatoptions[i],"DSCO:",5) == 0 )
      ds_options = CSLAddString( ds_options,
                                 format->formatoptions[i] + 5 );
  }
  if( EQUAL(pszFormatName, "GeoJSON") && nMatchingFeatures >= 0 )
  {
      const char* pszNativeData =
        CSLFetchNameValueDef(layer_options, "NATIVE_DATA", "{}");
      if( pszNativeData[strlen(pszNativeData)-1] == '}' )
      {
          std::string tmpl(pszNativeData);
          tmpl.resize(tmpl.size() - 1);
          if( strlen(pszNativeData) > 2 )
              tmpl += ',';
          tmpl += "\"numberMatched\":";
          tmpl += std::to_string(nMatchingFeatures);
          tmpl += '}';
          layer_options = CSLSetNameValue(layer_options,
                                          "NATIVE_MEDIA_TYPE",
                                          "application/vnd.geo+json");
          layer_options = CSLSetNameValue(layer_options,
                                          "NATIVE_DATA",
                                          tmpl.c_str());
      }
  }
  if(!strcasecmp("true",msGetOutputFormatOption(format,"USE_FEATUREID","false"))) {
    bUseFeatureId = MS_TRUE;
  }

  /* ==================================================================== */
  /*      Determine the output datasource name to use.                    */
  /* ==================================================================== */
  storage = msGetOutputFormatOption( format, "STORAGE", "filesystem" );
  if( EQUAL(storage,"stream") && !msIO_isStdContext() ) {
    msIOContext *ioctx = msIO_getHandler (stdout);
    if( ioctx != NULL )
        VSIStdoutSetRedirection( msOGRStdoutWriteFunction, (FILE*)ioctx );
    else
    /* bug #4858, streaming output won't work if standard output has been
     * redirected, we switch to memory output in this case
     */
    storage = "memory";
  }

  /* -------------------------------------------------------------------- */
  /*      Where are we putting stuff?                                     */
  /* -------------------------------------------------------------------- */
  if( EQUAL(storage,"filesystem") ) {
    base_dir[0] = '\0' ;
  } else if( EQUAL(storage,"memory") ) {
    strcpy( base_dir, "/vsimem/ogr_out/" );
  } else if( EQUAL(storage,"stream") ) {
    /* handled later */
  } else {
    msSetError( MS_MISCERR,
                "STORAGE=%s value not supported.",
                "msOGRWriteFromQuery()",
                storage );
    CSLDestroy(layer_options);
    CSLDestroy(ds_options);
    return MS_FAILURE;
  }

  /* -------------------------------------------------------------------- */
  /*      Create a subdirectory to handle this request.                   */
  /* -------------------------------------------------------------------- */
  if( !EQUAL(storage,"stream") ) {
    const char* dir_to_create;
    if (strlen(base_dir) > 0)
      request_dir = msTmpFile(map, NULL, base_dir, "" );
    else
      request_dir = msTmpFile(map, NULL, NULL, "" );

    if( request_dir[strlen(request_dir)-1] == '.' )
      request_dir[strlen(request_dir)-1] = '\0';

    dir_to_create = request_dir;
    /* Workaround issue in GDAL versions released at this time :
     * GDAL issue fixed per https://trac.osgeo.org/gdal/ticket/6991 */
    if( EQUAL(storage,"memory") && EQUAL(format->driver+4, "ESRI Shapefile") )
    {
        dir_to_create = base_dir;
    }

    if( VSIMkdir( dir_to_create, 0777 ) != 0 ) {
      msSetError( MS_MISCERR,
                  "Attempt to create directory '%s' failed.",
                  "msOGRWriteFromQuery()",
                  dir_to_create );
      msFree(request_dir);
      CSLDestroy(layer_options);
      CSLDestroy(ds_options);
      return MS_FAILURE;
    }
  }
  /*  else handled later */

  /* -------------------------------------------------------------------- */
  /*      Setup the full datasource name.                                 */
  /* -------------------------------------------------------------------- */
  form = msGetOutputFormatOption( format, "FORM", "zip" );

  if( EQUAL(form,"zip") )
    fo_filename = msGetOutputFormatOption( format, "FILENAME", "result.zip" );
  else
    fo_filename = msGetOutputFormatOption( format, "FILENAME", "result.dat" );

  /* Validate that the filename does not contain any directory */
  /* information, which might lead to removal of unwanted files. (#4086) */
  if( strchr(fo_filename, '/') != NULL || strchr(fo_filename, ':') != NULL ||
        strchr(fo_filename, '\\') != NULL ) {
    msSetError( MS_MISCERR,
           "Invalid value for FILENAME option. "
           "It must not contain any directory information.",
           "msOGRWriteFromQuery()" );
    msFree(request_dir);
    CSLDestroy(layer_options);
    CSLDestroy(ds_options);
    return MS_FAILURE;
  }

  if (!EQUAL(storage, "stream")) {
    if (!msBuildPath(datasource_name, request_dir, fo_filename)) {
      msFree(request_dir);
      CSLDestroy(layer_options);
      CSLDestroy(ds_options);
      return MS_FAILURE;
    }

    if( EQUAL(form,"zip") )
    {
      /* if generating a zip file, remove the zip extension for the internal */
      /* filename */
      if( EQUAL(CPLGetExtension(datasource_name), "zip") ) {
        *strrchr(datasource_name, '.') = '\0';
      }

      /* and add .dat extension if user didn't provide another extension */
      if (EQUAL(CPLGetExtension(datasource_name), "")) {
        strlcat(datasource_name, ".dat", sizeof(datasource_name));
      }
    }

    /* Shapefile and MapInfo driver only properly work with multiple layers */
    /* if the output dataset name is a directory */
    if( EQUAL(format->driver+4, "ESRI Shapefile") ||
        EQUAL(format->driver+4, "MapInfo File") )
    {
        bDataSourceNameIsRequestDir = TRUE;
        strcpy(datasource_name, request_dir);
    }
  }
  else
    strcpy( datasource_name, "/vsistdout/" );

  msFree( request_dir );
  request_dir = NULL;

  /* -------------------------------------------------------------------- */
  /*      Emit content type headers for stream output now.                */
  /* -------------------------------------------------------------------- */
  if( EQUAL(storage,"stream") ) {
    if( sendheaders && format->mimetype ) {
      msIO_setHeader("Content-Type","%s",format->mimetype);
      msIO_sendHeaders();
    } else
      msIO_fprintf( stdout, "%c", 10 );
  }

  /* ==================================================================== */
  /*      Create the datasource.                                          */
  /* ==================================================================== */
  hDS = OGR_Dr_CreateDataSource( hDriver,  datasource_name, ds_options );
  CSLDestroy( ds_options );

  if( hDS == NULL ) {
    msOGRCleanupDS( datasource_name );
    msSetError( MS_MISCERR,
                "OGR CreateDataSource failed for '%s' with driver '%s'.",
                "msOGRWriteFromQuery()",
                datasource_name,
                format->driver+4 );
    CSLDestroy(layer_options);
    return MS_FAILURE;
  }

  /* ==================================================================== */
  /*      Process each layer with a resultset.                            */
  /* ==================================================================== */
  for( iLayer = 0; iLayer < map->numlayers; iLayer++ ) {
    int status = 0;
    layerObj *layer = GET_LAYER(map, iLayer);
    shapeObj resultshape;
    OGRLayerH hOGRLayer;
    OGRwkbGeometryType eGeomType;
    OGRSpatialReferenceH srs = NULL;
    gmlItemListObj *item_list = NULL;
    const char *value;
    char *pszWKT;
    int  nFirstOGRFieldIndex = -1;
    const char *pszFeatureid = NULL;

    if( !layer->resultcache )
      continue;

    /* -------------------------------------------------------------------- */
    /*      Will we need to reproject?                                      */
    /* -------------------------------------------------------------------- */
    if(layer->transform == MS_TRUE)
        layer->project = msProjectionsDiffer(&(layer->projection),
                               &(layer->map->projection));

    /* -------------------------------------------------------------------- */
    /*      Establish the geometry type to use for the created layer.       */
    /*      First we consult the wfs_geomtype field and fallback to         */
    /*      deriving something from the type of the mapserver layer.        */
    /* -------------------------------------------------------------------- */
    value = msOWSLookupMetadata(&(layer->metadata), "FOG", "geomtype");
    if( value == NULL ) {
      if( layer->type == MS_LAYER_POINT )
        value = "Point";
      else if( layer->type == MS_LAYER_LINE )
        value = "LineString";
      else if( layer->type == MS_LAYER_POLYGON )
        value = "Polygon";
      else
        value = "Geometry";
    }

    if(bUseFeatureId)
      pszFeatureid = msOWSLookupMetadata(&(layer->metadata), "FOG", "featureid");

    if( strcasecmp(value,"Point") == 0 )
      eGeomType = wkbPoint;
    else if( strcasecmp(value,"LineString") == 0 )
      eGeomType = wkbLineString;
    else if( strcasecmp(value,"Polygon") == 0 )
      eGeomType = wkbPolygon;
    else if( strcasecmp(value,"MultiPoint") == 0 )
      eGeomType = wkbMultiPoint;
    else if( strcasecmp(value,"MultiLineString") == 0 )
      eGeomType = wkbMultiLineString;
    else if( strcasecmp(value,"MultiPolygon") == 0 )
      eGeomType = wkbMultiPolygon;
    else if( strcasecmp(value,"GeometryCollection") == 0 )
      eGeomType = wkbGeometryCollection;
    else if( strcasecmp(value,"Point25D") == 0 )
      eGeomType = wkbPoint25D;
    else if( strcasecmp(value,"LineString25D") == 0 )
      eGeomType = wkbLineString25D;
    else if( strcasecmp(value,"Polygon25D") == 0 )
      eGeomType = wkbPolygon25D;
    else if( strcasecmp(value,"MultiPoint25D") == 0 )
      eGeomType = wkbMultiPoint25D;
    else if( strcasecmp(value,"MultiLineString25D") == 0 )
      eGeomType = wkbMultiLineString25D;
    else if( strcasecmp(value,"MultiPolygon25D") == 0 )
      eGeomType = wkbMultiPolygon25D;
    else if( strcasecmp(value,"GeometryCollection25D") == 0 )
      eGeomType = wkbGeometryCollection25D;
    else if( strcasecmp(value,"Unknown") == 0
             || strcasecmp(value,"Geometry") == 0 )
      eGeomType = wkbUnknown;
    else if( strcasecmp(value,"None") == 0 )
      eGeomType = wkbNone;
    else
      eGeomType = wkbUnknown;

    /* -------------------------------------------------------------------- */
    /*      Create a spatial reference.                                     */
    /* -------------------------------------------------------------------- */
    pszWKT = msProjectionObj2OGCWKT( &(map->projection) );
    if( pszWKT != NULL ) {
      srs = OSRNewSpatialReference( pszWKT );
      msFree( pszWKT );
    }

    /* -------------------------------------------------------------------- */
    /*      Create the corresponding OGR Layer.                             */
    /* -------------------------------------------------------------------- */
    hOGRLayer = OGR_DS_CreateLayer( hDS, layer->name, srs, eGeomType,
                                    layer_options );
    if( hOGRLayer == NULL ) {
      OGR_DS_Destroy( hDS );
      msOGRCleanupDS( datasource_name );
      msSetError( MS_MISCERR,
                  "OGR OGR_DS_CreateLayer failed for layer '%s' with driver '%s'.",
                  "msOGRWriteFromQuery()",
                  layer->name,
                  format->driver+4 );
      CSLDestroy(layer_options);
      return MS_FAILURE;
    }

    if( srs != NULL )
      OSRRelease( srs );

    /* -------------------------------------------------------------------- */
    /*      Create appropriate attributes on this layer.                    */
    /* -------------------------------------------------------------------- */
    item_list = msGMLGetItems( layer, "G" );
    assert( item_list->numitems == layer->numitems );

    for( i = 0; i < layer->numitems; i++ ) {
      OGRFieldDefnH hFldDefn;
      OGRErr eErr;
      const char *name;
      gmlItemObj *item = item_list->items + i;
      OGRFieldType eType;

      if( !item->visible )
        continue;

      if( item->alias )
        name = item->alias;
      else
        name = item->name;

      if( item->type == NULL )
        eType = OFTString;
      else if( EQUAL(item->type,"Integer") )
        eType = OFTInteger;
      else if( EQUAL(item->type,"Long") )
        eType = OFTInteger64;
      else if( EQUAL(item->type,"Real") )
        eType = OFTReal;
      else if( EQUAL(item->type,"Character") )
        eType = OFTString;
      else if( EQUAL(item->type,"Date") )
        eType = OFTDate;
      else if( EQUAL(item->type,"Time") )
        eType = OFTTime;
      else if( EQUAL(item->type,"DateTime") )
        eType = OFTDateTime;
      else if( EQUAL(item->type,"Boolean") )
        eType = OFTInteger;
      else
        eType = OFTString;

      hFldDefn = OGR_Fld_Create( name, eType );

      if( item->width != 0 )
        OGR_Fld_SetWidth( hFldDefn, item->width );
      if( item->precision != 0 )
        OGR_Fld_SetPrecision( hFldDefn, item->precision );

      eErr = OGR_L_CreateField( hOGRLayer, hFldDefn, TRUE );
      OGR_Fld_Destroy( hFldDefn );

      if( eErr != OGRERR_NONE ) {
        msSetError( MS_OGRERR,
                    "Failed to create field '%s' in output feature schema:\n%s",
                    "msOGRWriteFromQuery()",
                    layer->items[i],
                    CPLGetLastErrorMsg() );

        OGR_DS_Destroy( hDS );
        msOGRCleanupDS( datasource_name );
        msGMLFreeItems(item_list);
        CSLDestroy(layer_options);
        return MS_FAILURE;
      }

      /* The index of the first field we create is not necessarily 0 */
      if( nFirstOGRFieldIndex < 0 )
          nFirstOGRFieldIndex = OGR_FD_GetFieldCount(
                                        OGR_L_GetLayerDefn( hOGRLayer ) ) - 1;
    }

    /* -------------------------------------------------------------------- */
    /*      Setup joins if needed.  This is likely untested.                */
    /* -------------------------------------------------------------------- */
    if(layer->numjoins > 0) {
      int j;
      for(j=0; j<layer->numjoins; j++) {
        status = msJoinConnect(layer, &(layer->joins[j]));
        if(status != MS_SUCCESS) {
          OGR_DS_Destroy( hDS );
          msOGRCleanupDS( datasource_name );
          msGMLFreeItems(item_list);
          CSLDestroy(layer_options);
          return status;
        }
      }
    }

    msInitShape( &resultshape );

    /* -------------------------------------------------------------------- */
    /*      Loop over all the shapes in the resultcache.                    */
    /* -------------------------------------------------------------------- */
    for(i=0; i < layer->resultcache->numresults; i++) {

      msFreeShape(&resultshape); /* init too */

      /*
      ** Read the shape.
      */
      if( layer->resultcache->results[i].shape )
      {
          /* msDebug("Using cached shape %ld\n", layer->resultcache->results[i].shapeindex); */
          status = msCopyShape(layer->resultcache->results[i].shape, &resultshape);
      }
      else
      {
          status = msLayerGetShape(layer, &resultshape, &(layer->resultcache->results[i]));
      }
      
      if(status != MS_SUCCESS) {
          OGR_DS_Destroy( hDS );
          msOGRCleanupDS( datasource_name );
          msGMLFreeItems(item_list);
          msFreeShape(&resultshape);
          CSLDestroy(layer_options);
          return status;
      }      

      /*
      ** Perform classification, and some annotation related magic.
      */
      resultshape.classindex =
        msShapeGetClass(layer, map, &resultshape, NULL, -1);

      if( resultshape.classindex >= 0
          && (layer->_class[resultshape.classindex]->text.string
              || layer->labelitem)
          && layer->_class[resultshape.classindex]->numlabels > 0
          && layer->_class[resultshape.classindex]->labels[0]->size != -1 ) {
        resultshape.text = msShapeGetLabelAnnotation(layer,&resultshape,layer->_class[resultshape.classindex]->labels[0]);
      }

      /*
      ** prepare any necessary JOINs here (one-to-one only)
      */
      if( layer->numjoins > 0) {
        int j;

        for(j=0; j < layer->numjoins; j++) {
          if(layer->joins[j].type == MS_JOIN_ONE_TO_ONE) {
            msJoinPrepare(&(layer->joins[j]), &resultshape);
            msJoinNext(&(layer->joins[j])); /* fetch the first row */
          }
        }
      }

      if( layer->project ) {
        if( layer->reprojectorLayerToMap == NULL )
        {
            layer->reprojectorLayerToMap = msProjectCreateReprojector(
                &layer->projection, &layer->map->projection);
        }
        if( layer->reprojectorLayerToMap )
            status = msProjectShapeEx(layer->reprojectorLayerToMap, &resultshape);
        else
            status = MS_FAILURE;
      }

      /*
      ** Write out the feature to OGR.
      */

      if( status == MS_SUCCESS )
        status = msOGRWriteShape( layer, hOGRLayer, &resultshape,
                                  item_list, nFirstOGRFieldIndex, pszFeatureid );

      if(status != MS_SUCCESS) {
        OGR_DS_Destroy( hDS );
        msOGRCleanupDS( datasource_name );
        msGMLFreeItems(item_list);
        msFreeShape(&resultshape);
        CSLDestroy(layer_options);
        return status;
      }
    }

    msGMLFreeItems(item_list);
    msFreeShape(&resultshape); /* init too */
  }

  /* -------------------------------------------------------------------- */
  /*      Close the datasource.                                           */
  /* -------------------------------------------------------------------- */
  OGR_DS_Destroy( hDS );

  CSLDestroy( layer_options );

  /* -------------------------------------------------------------------- */
  /*      Get list of resulting files.                                    */
  /* -------------------------------------------------------------------- */

  if( EQUAL(form,"simple") ) {
    file_list = CSLAddString( NULL, datasource_name );
  } else {
    char datasource_path[MS_MAXPATHLEN];

    if( bDataSourceNameIsRequestDir )
        file_list = msOGRRecursiveFileList( datasource_name );
    else
    {
        strncpy( datasource_path, CPLGetPath( datasource_name ), MS_MAXPATHLEN-1 );
        file_list = msOGRRecursiveFileList( datasource_path );
    }
  }

  /* -------------------------------------------------------------------- */
  /*      If our "storage" is stream then the output has already been     */
  /*      sent back to the client and we don't need to copy it now.       */
  /* -------------------------------------------------------------------- */
  if( EQUAL(storage,"stream") ) {
    /* already done */
  }

  /* -------------------------------------------------------------------- */
  /*      Handle case of simple file written to stdout.                   */
  /* -------------------------------------------------------------------- */
  else if( EQUAL(form,"simple") ) {
    char buffer[1024];
    int  bytes_read;
    FILE *fp;
    const char *jsonp;

    jsonp = msGetOutputFormatOption( format, "JSONP", NULL );
    if( sendheaders ) {
      if( !jsonp )
      msIO_setHeader("Content-Disposition","attachment; filename=%s",
                     CPLGetFilename( file_list[0] ) );
      if( format->mimetype )
        msIO_setHeader("Content-Type","%s",format->mimetype);
      msIO_sendHeaders();
    } else
      msIO_fprintf( stdout, "%c", 10 );

    fp = VSIFOpenL( file_list[0], "r" );
    if( fp == NULL ) {
      msSetError( MS_MISCERR,
                  "Failed to open result file '%s'.",
                  "msOGRWriteFromQuery()",
                  file_list[0] );
      msOGRCleanupDS( datasource_name );
      return MS_FAILURE;
    }

    if( jsonp != NULL ) msIO_fprintf( stdout, "%s(", jsonp );

    while( (bytes_read = VSIFReadL( buffer, 1, sizeof(buffer), fp )) > 0 )
      msIO_fwrite( buffer, 1, bytes_read, stdout );
    VSIFCloseL( fp );

    if (jsonp != NULL) msIO_fprintf( stdout, ");\n" );
  }

  /* -------------------------------------------------------------------- */
  /*      Handle the case of a multi-part result.                         */
  /* -------------------------------------------------------------------- */
  else if( EQUAL(form,"multipart") ) {
    char **papszAdditionalFiles;
    static const char *boundary = "xxOGRBoundaryxx";
    msIO_setHeader("Content-Type","multipart/mixed; boundary=%s",boundary);
    msIO_sendHeaders();
    msIO_fprintf(stdout,"--%s\r\n",boundary );

    papszAdditionalFiles = msOGROutputGetAdditonalFiles(map);
    file_list = msCSLConcatenate(file_list, papszAdditionalFiles);
    CSLDestroy(papszAdditionalFiles);

    for( i = 0; file_list != NULL && file_list[i] != NULL; i++ ) {
      FILE *fp;
      int bytes_read;
      char buffer[1024];

      if( sendheaders )
        msIO_fprintf( stdout,
                      "Content-Disposition: attachment; filename=%s\r\n"
                      "Content-Type: application/binary\r\n"
                      "Content-Transfer-Encoding: binary\r\n\r\n",
                      CPLGetFilename( file_list[i] ));


      fp = VSIFOpenL( file_list[i], "r" );
      if( fp == NULL ) {
        msSetError( MS_MISCERR,
                    "Failed to open result file '%s'.",
                    "msOGRWriteFromQuery()",
                    file_list[0] );
        msOGRCleanupDS( datasource_name );
        return MS_FAILURE;
      }

      while( (bytes_read = VSIFReadL( buffer, 1, sizeof(buffer), fp )) > 0 )
        msIO_fwrite( buffer, 1, bytes_read, stdout );
      VSIFCloseL( fp );

      if (file_list[i+1] == NULL)
        msIO_fprintf( stdout, "\r\n--%s--\r\n", boundary );
      else
        msIO_fprintf( stdout, "\r\n--%s\r\n", boundary );
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Handle the case of a zip file result.                           */
  /* -------------------------------------------------------------------- */
  else if( EQUAL(form,"zip") ) {
    FILE *fp;
    char *zip_filename = msTmpFile(map, NULL, "/vsimem/ogrzip/", "zip" );
    void *hZip;
    int bytes_read;
    char buffer[1024];
    char **papszAdditionalFiles;

    hZip = CPLCreateZip( zip_filename, NULL );

    papszAdditionalFiles = msOGROutputGetAdditonalFiles(map);
    file_list = msCSLConcatenate(file_list, papszAdditionalFiles);
    CSLDestroy(papszAdditionalFiles);

    for( i = 0; file_list != NULL && file_list[i] != NULL; i++ ) {

      CPLCreateFileInZip( hZip, CPLGetFilename(file_list[i]), NULL );

      fp = VSIFOpenL( file_list[i], "r" );
      if( fp == NULL ) {
        CPLCloseZip( hZip );
        msSetError( MS_MISCERR,
                    "Failed to open result file '%s'.",
                    "msOGRWriteFromQuery()",
                    file_list[0] );
        msOGRCleanupDS( datasource_name );
        return MS_FAILURE;
      }

      while( (bytes_read = VSIFReadL( buffer, 1, sizeof(buffer), fp )) > 0 ) {
        CPLWriteFileInZip( hZip, buffer, bytes_read );
      }
      VSIFCloseL( fp );

      CPLCloseFileInZip( hZip );
    }
    CPLCloseZip( hZip );

    if( sendheaders ) {
      const char* zip_filename = fo_filename;
      /* Make sure the filename is ended by .zip */
      if( !EQUAL(CPLGetExtension(zip_filename), "zip") &&
          !EQUAL(CPLGetExtension(zip_filename), "kmz") )
          zip_filename = CPLFormFilename(NULL, fo_filename, "zip");
      msIO_setHeader("Content-Disposition","attachment; filename=%s",zip_filename);
      msIO_setHeader("Content-Type","application/zip");
      msIO_sendHeaders();
    }

    fp = VSIFOpenL( zip_filename, "r" );
    if( fp == NULL ) {
      msSetError( MS_MISCERR,
                  "Failed to open zip file '%s'.",
                  "msOGRWriteFromQuery()",
                  file_list[0] );
      msOGRCleanupDS( datasource_name );
      return MS_FAILURE;
    }

    while( (bytes_read = VSIFReadL( buffer, 1, sizeof(buffer), fp )) > 0 )
      msIO_fwrite( buffer, 1, bytes_read, stdout );
    VSIFCloseL( fp );

    msFree( zip_filename );
  }

  /* -------------------------------------------------------------------- */
  /*      Handle illegal form value.                                      */
  /* -------------------------------------------------------------------- */
  else {
    msSetError( MS_MISCERR, "Unsupported FORM=%s value.",
                "msOGRWriteFromQuery()", form );
    msOGRCleanupDS( datasource_name );
    return MS_FAILURE;
  }

  msOGRCleanupDS( datasource_name );

  CSLDestroy( file_list );

  return MS_SUCCESS;
}

/************************************************************************/
/*                     msPopulateRenderVTableOGR()                      */
/************************************************************************/

int msPopulateRendererVTableOGR( rendererVTableObj *renderer )
{
  /* we aren't really a normal renderer so we leave everything default */
  return MS_SUCCESS;
}

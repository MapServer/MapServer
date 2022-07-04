/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  OGC Web Map Context implementation
 * Author:   Julien-Samuel Lacroix, DM Solutions Group (lacroix@dmsolutions.ca)
 *
 **********************************************************************
 * Copyright (c) 2002-2003, Julien-Samuel Lacroix, DM Solutions Group Inc
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
#include "mapows.h"

#include "cpl_vsi.h"


#if defined(USE_WMS_LYR)

/* There is a dependency to GDAL/OGR for the GML driver and MiniXML parser */
#include "cpl_minixml.h"

#endif

/* msGetMapContextFileText()
**
** Read a file and return is content
**
** Take the filename in argument
** Return value must be freed by caller
*/
char * msGetMapContextFileText(char *filename)
{
  char *pszBuffer;
  VSILFILE *stream;
  int  nLength;

  /* open file */
  if(filename != NULL && strlen(filename) > 0) {
    stream = VSIFOpenL(filename, "rb");
    if(!stream) {
      msSetError(MS_IOERR, "(%s)", "msGetMapContextFileText()", filename);
      return NULL;
    }
  } else {
    msSetError(MS_IOERR, "(%s)", "msGetMapContextFileText()", filename);
    return NULL;
  }

  VSIFSeekL( stream, 0, SEEK_END );
  nLength = (int) VSIFTellL( stream );
  VSIFSeekL( stream, 0, SEEK_SET );

  pszBuffer = (char *) malloc(nLength+1);
  if( pszBuffer == NULL ) {
    msSetError(MS_MEMERR, "(%s)", "msGetMapContextFileText()", filename);
    VSIFCloseL( stream );
    return NULL;
  }

  if(VSIFReadL( pszBuffer, nLength, 1, stream ) == 0) {
    free( pszBuffer );
    VSIFCloseL( stream );
    msSetError(MS_IOERR, "(%s)", "msGetMapContextFileText()", filename);
    return NULL;
  }
  pszBuffer[nLength] = '\0';

  VSIFCloseL( stream );

  return pszBuffer;
}


#if defined(USE_WMS_LYR)

/*
**msGetMapContextXMLHashValue()
**
**Get the xml value and put it in the hash table
**
*/
int msGetMapContextXMLHashValue( CPLXMLNode *psRoot, const char *pszXMLPath,
                                 hashTableObj *metadata, char *pszMetadata )
{
  char *pszValue;

  pszValue = (char*)CPLGetXMLValue( psRoot, pszXMLPath, NULL);
  if(pszValue != NULL) {
    if( metadata != NULL ) {
      msInsertHashTable(metadata, pszMetadata, pszValue );
    } else {
      return MS_FAILURE;
    }
  } else {
    return MS_FAILURE;
  }

  return MS_SUCCESS;
}

/*
**msGetMapContextXMLHashValue()
**
**Get the xml value and put it in the hash table
**
*/
int msGetMapContextXMLHashValueDecode( CPLXMLNode *psRoot,
                                       const char *pszXMLPath,
                                       hashTableObj *metadata,
                                       char *pszMetadata )
{
  char *pszValue;

  pszValue = (char*)CPLGetXMLValue( psRoot, pszXMLPath, NULL);
  if(pszValue != NULL) {
    if( metadata != NULL ) {
      msDecodeHTMLEntities(pszValue);
      msInsertHashTable(metadata, pszMetadata, pszValue );
    } else {
      return MS_FAILURE;
    }
  } else {
    return MS_FAILURE;
  }

  return MS_SUCCESS;
}


/*
**msGetMapContextXMLStringValue()
**
**Get the xml value and put it in the string field
**
*/
int msGetMapContextXMLStringValue( CPLXMLNode *psRoot, char *pszXMLPath,
                                   char **pszField)
{
  char *pszValue;

  pszValue = (char*)CPLGetXMLValue( psRoot, pszXMLPath, NULL);
  if(pszValue != NULL) {
    if( pszField != NULL ) {
      *pszField = msStrdup(pszValue);
    } else {
      return MS_FAILURE;
    }
  } else {
    return MS_FAILURE;
  }

  return MS_SUCCESS;
}


/*
**msGetMapContextXMLStringValue()
**
**Get the xml value and put it in the string field
**
*/
int msGetMapContextXMLStringValueDecode( CPLXMLNode *psRoot, char *pszXMLPath,
    char **pszField)
{
  char *pszValue;

  pszValue = (char*)CPLGetXMLValue( psRoot, pszXMLPath, NULL);
  if(pszValue != NULL) {
    if( pszField != NULL ) {
      msDecodeHTMLEntities(pszValue);
      *pszField = msStrdup(pszValue);
    } else {
      return MS_FAILURE;
    }
  } else {
    return MS_FAILURE;
  }

  return MS_SUCCESS;
}


/*
**msGetMapContextXMLFloatValue()
**
**Get the xml value and put it in the string field
**
*/
int msGetMapContextXMLFloatValue( CPLXMLNode *psRoot, char *pszXMLPath,
                                  double *pszField)
{
  char *pszValue;

  pszValue = (char*)CPLGetXMLValue( psRoot, pszXMLPath, NULL);
  if(pszValue != NULL) {
    if( pszField != NULL ) {
      *pszField = atof(pszValue);
    } else {
      return MS_FAILURE;
    }
  } else {
    return MS_FAILURE;
  }

  return MS_SUCCESS;
}

/*
** msLoadMapContextURLELements
**
** Take a Node and get the width, height, format and href from it.
** Then put this info in metadatas.
*/
int msLoadMapContextURLELements( CPLXMLNode *psRoot, hashTableObj *metadata,
                                 const char *pszMetadataRoot)
{
  char *pszMetadataName;

  if( psRoot == NULL || metadata == NULL || pszMetadataRoot == NULL )
    return MS_FAILURE;

  pszMetadataName = (char*) malloc( strlen(pszMetadataRoot) + 10 );

  sprintf( pszMetadataName, "%s_width", pszMetadataRoot );
  msGetMapContextXMLHashValue( psRoot, "width", metadata, pszMetadataName );

  sprintf( pszMetadataName, "%s_height", pszMetadataRoot );
  msGetMapContextXMLHashValue( psRoot, "height", metadata, pszMetadataName );

  sprintf( pszMetadataName, "%s_format", pszMetadataRoot );
  msGetMapContextXMLHashValue( psRoot, "format", metadata, pszMetadataName );

  sprintf( pszMetadataName, "%s_href", pszMetadataRoot );
  msGetMapContextXMLHashValue( psRoot, "OnlineResource.xlink:href", metadata,
                               pszMetadataName );

  free(pszMetadataName);

  return MS_SUCCESS;
}

/* msLoadMapContextKeyword
**
** Put the keywords from a XML node and put them in a metadata.
** psRoot should be set to keywordlist
*/
int msLoadMapContextListInMetadata( CPLXMLNode *psRoot, hashTableObj *metadata,
                                    char *pszXMLName, char *pszMetadataName,
                                    char *pszHashDelimiter)
{
  const char *pszHash, *pszXMLValue;
  char *pszMetadata;

  if(psRoot == NULL || psRoot->psChild == NULL ||
      metadata == NULL || pszMetadataName == NULL || pszXMLName == NULL)
    return MS_FAILURE;

  /* Pass from KeywordList to Keyword level */
  psRoot = psRoot->psChild;

  /* Loop on all elements and append keywords to the hash table */
  while (psRoot) {
    if (psRoot->psChild && strcasecmp(psRoot->pszValue, pszXMLName) == 0) {
      pszXMLValue = psRoot->psChild->pszValue;
      pszHash = msLookupHashTable(metadata, pszMetadataName);
      if (pszHash != NULL) {
        pszMetadata = (char*)malloc(strlen(pszHash)+
                                    strlen(pszXMLValue)+2);
        if(pszHashDelimiter == NULL)
          sprintf( pszMetadata, "%s%s", pszHash, pszXMLValue );
        else
          sprintf( pszMetadata, "%s%s%s", pszHash, pszHashDelimiter,
                   pszXMLValue );
        msInsertHashTable(metadata, pszMetadataName, pszMetadata);
        free(pszMetadata);
      } else
        msInsertHashTable(metadata, pszMetadataName, pszXMLValue);
    }
    psRoot = psRoot->psNext;
  }

  return MS_SUCCESS;
}

/* msLoadMapContextContactInfo
**
** Put the Contact informations from a XML node and put them in a metadata.
**
*/
int msLoadMapContextContactInfo( CPLXMLNode *psRoot, hashTableObj *metadata )
{
  if(psRoot == NULL || metadata == NULL)
    return MS_FAILURE;

  /* Contact Person primary */
  msGetMapContextXMLHashValue(psRoot,
                              "ContactPersonPrimary.ContactPerson",
                              metadata, "wms_contactperson");
  msGetMapContextXMLHashValue(psRoot,
                              "ContactPersonPrimary.ContactOrganization",
                              metadata, "wms_contactorganization");
  /* Contact Position */
  msGetMapContextXMLHashValue(psRoot,
                              "ContactPosition",
                              metadata, "wms_contactposition");
  /* Contact Address */
  msGetMapContextXMLHashValue(psRoot, "ContactAddress.AddressType",
                              metadata, "wms_addresstype");
  msGetMapContextXMLHashValue(psRoot, "ContactAddress.Address",
                              metadata, "wms_address");
  msGetMapContextXMLHashValue(psRoot, "ContactAddress.City",
                              metadata, "wms_city");
  msGetMapContextXMLHashValue(psRoot, "ContactAddress.StateOrProvince",
                              metadata, "wms_stateorprovince");
  msGetMapContextXMLHashValue(psRoot, "ContactAddress.PostCode",
                              metadata, "wms_postcode");
  msGetMapContextXMLHashValue(psRoot, "ContactAddress.Country",
                              metadata, "wms_country");

  /* Others */
  msGetMapContextXMLHashValue(psRoot, "ContactVoiceTelephone",
                              metadata, "wms_contactvoicetelephone");
  msGetMapContextXMLHashValue(psRoot, "ContactFacsimileTelephone",
                              metadata, "wms_contactfacsimiletelephone");
  msGetMapContextXMLHashValue(psRoot, "ContactElectronicMailAddress",
                              metadata, "wms_contactelectronicmailaddress");

  return MS_SUCCESS;
}

/*
** msLoadMapContextLayerFormat
**
**
*/
int msLoadMapContextLayerFormat(CPLXMLNode *psFormat, layerObj *layer)
{
  const char *pszValue;
  char *pszValue1;
  const char* pszHash;

  if(psFormat->psChild != NULL &&
      strcasecmp(psFormat->pszValue, "Format") == 0 ) {
    if(psFormat->psChild->psNext == NULL)
      pszValue = psFormat->psChild->pszValue;
    else
      pszValue = psFormat->psChild->psNext->pszValue;
  } else
    pszValue = NULL;

  if(pszValue != NULL && strcasecmp(pszValue, "") != 0) {
    /* wms_format */
    pszValue1 = (char*)CPLGetXMLValue(psFormat,
                                      "current", NULL);
    if(pszValue1 != NULL &&
        (strcasecmp(pszValue1, "1") == 0 || strcasecmp(pszValue1, "true")==0))
      msInsertHashTable(&(layer->metadata),
                        "wms_format", pszValue);
    /* wms_formatlist */
    pszHash = msLookupHashTable(&(layer->metadata),
                                "wms_formatlist");
    if(pszHash != NULL) {
      pszValue1 = (char*)malloc(strlen(pszHash)+
                                strlen(pszValue)+2);
      sprintf(pszValue1, "%s,%s", pszHash, pszValue);
      msInsertHashTable(&(layer->metadata),
                        "wms_formatlist", pszValue1);
      free(pszValue1);
    } else
      msInsertHashTable(&(layer->metadata),
                        "wms_formatlist", pszValue);
  }

  /* Make sure selected format is supported or select another
   * supported format.  Note that we can efficiently do this
   * only for GIF/PNG/JPEG, can't try to handle all GDAL
   * formats.
   */
  pszValue = msLookupHashTable(&(layer->metadata), "wms_format");

  if (
    pszValue && (
#if !(defined USE_PNG)
    strcasecmp(pszValue, "image/png") == 0 ||
    strcasecmp(pszValue, "PNG") == 0 ||
#endif
#if !(defined USE_JPEG)
    strcasecmp(pszValue, "image/jpeg") == 0 ||
    strcasecmp(pszValue, "JPEG") == 0 ||
#endif
    0 )) {
    char **papszList=NULL;
    int i, numformats=0;

    pszValue = msLookupHashTable(&(layer->metadata),
                                 "wms_formatlist");

    papszList = msStringSplit(pszValue, ',', &numformats);
    for(i=0; i < numformats; i++) {
      if (
#if (defined USE_PNG)
        strcasecmp(papszList[i], "image/png") == 0 ||
        strcasecmp(papszList[i], "PNG") == 0 ||
#endif
#if (defined USE_JPEG)
        strcasecmp(papszList[i], "image/jpeg") == 0 ||
        strcasecmp(papszList[i], "JPEG") == 0 ||
#endif
#ifdef USE_GD_GIF
        strcasecmp(papszList[i], "image/gif") == 0 ||
        strcasecmp(papszList[i], "GIF") == 0 ||
#endif
        0 ) {
        /* Found a match */
        msInsertHashTable(&(layer->metadata),
                          "wms_format", papszList[i]);
        break;
      }
    }
    if(papszList)
      msFreeCharArray(papszList, numformats);

  } /* end if unsupported format */

  return MS_SUCCESS;
}

int msLoadMapContextLayerStyle(CPLXMLNode *psStyle, layerObj *layer,
                               int nStyle)
{
  char *pszValue, *pszValue1, *pszValue2;
  const char *pszHash;
  char* pszStyle=NULL;
  char *pszStyleName;
  CPLXMLNode *psStyleSLDBody;

  pszStyleName =(char*)CPLGetXMLValue(psStyle,"Name",NULL);
  if(pszStyleName == NULL) {
    pszStyleName = (char*)malloc(20);
    sprintf(pszStyleName, "Style{%d}", nStyle);
  } else
    pszStyleName = msStrdup(pszStyleName);

  /* wms_style */
  pszValue = (char*)CPLGetXMLValue(psStyle,"current",NULL);
  if(pszValue != NULL &&
      (strcasecmp(pszValue, "1") == 0 ||
       strcasecmp(pszValue, "true") == 0))
    msInsertHashTable(&(layer->metadata),
                      "wms_style", pszStyleName);
  /* wms_stylelist */
  pszHash = msLookupHashTable(&(layer->metadata),
                              "wms_stylelist");
  if(pszHash != NULL) {
    pszValue1 = (char*)malloc(strlen(pszHash)+
                              strlen(pszStyleName)+2);
    sprintf(pszValue1, "%s,%s", pszHash, pszStyleName);
    msInsertHashTable(&(layer->metadata),
                      "wms_stylelist", pszValue1);
    free(pszValue1);
  } else
    msInsertHashTable(&(layer->metadata),
                      "wms_stylelist", pszStyleName);

  /* Title */
  pszStyle = (char*)malloc(strlen(pszStyleName)+20);
  sprintf(pszStyle,"wms_style_%s_title",pszStyleName);

  if( msGetMapContextXMLHashValue(psStyle, "Title", &(layer->metadata),
                                  pszStyle) == MS_FAILURE )
    msInsertHashTable(&(layer->metadata), pszStyle, layer->name);

  free(pszStyle);

  /* SLD */
  pszStyle = (char*)malloc(strlen(pszStyleName)+15);
  sprintf(pszStyle, "wms_style_%s_sld", pszStyleName);

  msGetMapContextXMLHashValueDecode( psStyle, "SLD.OnlineResource.xlink:href",
                                     &(layer->metadata), pszStyle );
  free(pszStyle);

  /* SLDBODY */
  pszStyle = (char*)malloc(strlen(pszStyleName)+20);
  sprintf(pszStyle, "wms_style_%s_sld_body", pszStyleName);

  psStyleSLDBody = CPLGetXMLNode(psStyle, "SLD.StyledLayerDescriptor");
  /*some clients such as OpenLayers add a name space, which I believe is wrong but
   added this additional test for compatibility #3115*/
  if (psStyleSLDBody == NULL)
    psStyleSLDBody = CPLGetXMLNode(psStyle, "SLD.sld:StyledLayerDescriptor");

  if(psStyleSLDBody != NULL && &(layer->metadata) != NULL) {
    pszValue = CPLSerializeXMLTree(psStyleSLDBody);
    if(pszValue != NULL) {
      /* Before including SLDBody in the mapfile, we must replace the */
      /* double quote for single quote. This is to prevent having this: */
      /* "metadata" "<string attriute="ttt">" */
      char *c;
      for(c=pszValue; *c != '\0'; c++)
        if(*c == '"')
          *c = '\'';
      msInsertHashTable(&(layer->metadata), pszStyle, pszValue );
      msFree(pszValue);
    }
  }

  free(pszStyle);

  /* LegendURL */
  pszStyle = (char*) malloc(strlen(pszStyleName) + 25);

  sprintf( pszStyle, "wms_style_%s_legendurl",
           pszStyleName);
  msLoadMapContextURLELements( CPLGetXMLNode(psStyle, "LegendURL"),
                               &(layer->metadata), pszStyle );

  free(pszStyle);

  free(pszStyleName);

  /*  */
  /* Add the stylelist to the layer connection */
  /*  */
  if(msLookupHashTable(&(layer->metadata),
                       "wms_stylelist") == NULL) {
    if(layer->connection)
      pszValue = msStrdup(layer->connection);
    else
      pszValue = msStrdup( "" );
    pszValue1 = strstr(pszValue, "STYLELIST=");
    if(pszValue1 != NULL) {
      pszValue1 += 10;
      pszValue2 = strchr(pszValue, '&');
      if(pszValue2 != NULL)
        pszValue1[pszValue2-pszValue1] = '\0';
      msInsertHashTable(&(layer->metadata), "wms_stylelist",
                        pszValue1);
    }
    free(pszValue);
  }

  /*  */
  /* Add the style to the layer connection */
  /*  */
  if(msLookupHashTable(&(layer->metadata), "wms_style") == NULL) {
    if(layer->connection)
      pszValue = msStrdup(layer->connection);
    else
      pszValue = msStrdup( "" );
    pszValue1 = strstr(pszValue, "STYLE=");
    if(pszValue1 != NULL) {
      pszValue1 += 6;
      pszValue2 = strchr(pszValue, '&');
      if(pszValue2 != NULL)
        pszValue1[pszValue2-pszValue1] = '\0';
      msInsertHashTable(&(layer->metadata), "wms_style",
                        pszValue1);
    }
    free(pszValue);
  }

  return MS_SUCCESS;
}

int msLoadMapContextLayerDimension(CPLXMLNode *psDimension, layerObj *layer)
{
  char *pszValue;
  const char *pszHash;
  char *pszDimension=NULL, *pszDimensionName=NULL;

  pszDimensionName =(char*)CPLGetXMLValue(psDimension,"name",NULL);
  if(pszDimensionName == NULL) {
    return MS_FALSE;
  } else
    pszDimensionName = msStrdup(pszDimensionName);

  pszDimension = (char*)malloc(strlen(pszDimensionName)+50);

  /* wms_dimension: This is the current dimension */
  pszValue = (char*)CPLGetXMLValue(psDimension, "current", NULL);
  if(pszValue != NULL && (strcasecmp(pszValue, "1") == 0 ||
                          strcasecmp(pszValue, "true") == 0))
    msInsertHashTable(&(layer->metadata),
                      "wms_dimension", pszDimensionName);
  /* wms_dimensionlist */
  pszHash = msLookupHashTable(&(layer->metadata),
                              "wms_dimensionlist");
  if(pszHash != NULL) {
    pszValue = (char*)malloc(strlen(pszHash)+
                             strlen(pszDimensionName)+2);
    sprintf(pszValue, "%s,%s", pszHash, pszDimensionName);
    msInsertHashTable(&(layer->metadata),
                      "wms_dimensionlist", pszValue);
    free(pszValue);
  } else
    msInsertHashTable(&(layer->metadata),
                      "wms_dimensionlist", pszDimensionName);

  /* Units */
  sprintf(pszDimension, "wms_dimension_%s_units", pszDimensionName);
  msGetMapContextXMLHashValue(psDimension, "units", &(layer->metadata),
                              pszDimension);

  /* UnitSymbol */
  sprintf(pszDimension, "wms_dimension_%s_unitsymbol", pszDimensionName);
  msGetMapContextXMLHashValue(psDimension, "unitSymbol", &(layer->metadata),
                              pszDimension);

  /* userValue */
  sprintf(pszDimension, "wms_dimension_%s_uservalue", pszDimensionName);
  msGetMapContextXMLHashValue(psDimension, "userValue", &(layer->metadata),
                              pszDimension);
  if(strcasecmp(pszDimensionName, "time") == 0)
    msGetMapContextXMLHashValue(psDimension, "userValue", &(layer->metadata),
                                "wms_time");

  /* default */
  sprintf(pszDimension, "wms_dimension_%s_default", pszDimensionName);
  msGetMapContextXMLHashValue(psDimension, "default", &(layer->metadata),
                              pszDimension);

  /* multipleValues */
  sprintf(pszDimension, "wms_dimension_%s_multiplevalues", pszDimensionName);
  msGetMapContextXMLHashValue(psDimension, "multipleValues",&(layer->metadata),
                              pszDimension);

  /* nearestValue */
  sprintf(pszDimension, "wms_dimension_%s_nearestvalue", pszDimensionName);
  msGetMapContextXMLHashValue(psDimension, "nearestValue", &(layer->metadata),
                              pszDimension);

  free(pszDimension);

  free(pszDimensionName);

  return MS_SUCCESS;
}



/*
** msLoadMapContextGeneral
**
** Load the General block of the mapcontext document
*/
int msLoadMapContextGeneral(mapObj *map, CPLXMLNode *psGeneral,
                            CPLXMLNode *psMapContext, int nVersion,
                            char *filename)
{

  char *pszProj=NULL;
  char *pszValue, *pszValue1, *pszValue2;
  int nTmp;

  /* Projection */
  pszValue = (char*)CPLGetXMLValue(psGeneral,
                                   "BoundingBox.SRS", NULL);
  if(pszValue != NULL && !EQUAL(pszValue, "(null)")) {
    if(strncasecmp(pszValue, "AUTO:", 5) == 0) {
      pszProj = msStrdup(pszValue);
    } else {
      pszProj = (char*) malloc(sizeof(char)*(strlen(pszValue)+10));
      sprintf(pszProj, "init=epsg:%s", pszValue+5);
    }

    msInitProjection(&map->projection);
    map->projection.args[map->projection.numargs] = msStrdup(pszProj);
    map->projection.numargs++;
    msProcessProjection(&map->projection);

    if( (nTmp = GetMapserverUnitUsingProj(&(map->projection))) == -1) {
      free(pszProj);
      msSetError( MS_MAPCONTEXTERR,
                  "Unable to set units for projection '%s'",
                  "msLoadMapContext()", pszProj );
      return MS_FAILURE;
    } else {
      map->units = nTmp;
    }
    free(pszProj);
  } else {
    msDebug("Mandatory data General.BoundingBox.SRS missing in %s.",
            filename);
  }

  /* Extent */
  if( msGetMapContextXMLFloatValue(psGeneral, "BoundingBox.minx",
                                   &(map->extent.minx)) == MS_FAILURE) {
    msDebug("Mandatory data General.BoundingBox.minx missing in %s.",
            filename);
  }
  if( msGetMapContextXMLFloatValue(psGeneral, "BoundingBox.miny",
                                   &(map->extent.miny)) == MS_FAILURE) {
    msDebug("Mandatory data General.BoundingBox.miny missing in %s.",
            filename);
  }
  if( msGetMapContextXMLFloatValue(psGeneral, "BoundingBox.maxx",
                                   &(map->extent.maxx)) == MS_FAILURE) {
    msDebug("Mandatory data General.BoundingBox.maxx missing in %s.",
            filename);
  }
  if( msGetMapContextXMLFloatValue(psGeneral, "BoundingBox.maxy",
                                   &(map->extent.maxy)) == MS_FAILURE) {
    msDebug("Mandatory data General.BoundingBox.maxy missing in %s.",
            filename);
  }

  /* Title */
  if( msGetMapContextXMLHashValue(psGeneral, "Title",
                                  &(map->web.metadata), "wms_title") == MS_FAILURE) {
    if ( nVersion >= OWS_1_0_0 )
      msDebug("Mandatory data General.Title missing in %s.", filename);
    else {
      if( msGetMapContextXMLHashValue(psGeneral, "gml:name",
                                      &(map->web.metadata), "wms_title") == MS_FAILURE ) {
        if( nVersion < OWS_0_1_7 )
          msDebug("Mandatory data General.Title missing in %s.", filename);
        else
          msDebug("Mandatory data General.gml:name missing in %s.",
                  filename);
      }
    }
  }

  /* Name */
  if( nVersion >= OWS_1_0_0 ) {
    pszValue = (char*)CPLGetXMLValue(psMapContext,
                                     "id", NULL);
    if (pszValue)
      map->name = msStrdup(pszValue);
  } else {
    if(msGetMapContextXMLStringValue(psGeneral, "Name",
                                     &(map->name)) == MS_FAILURE) {
      msGetMapContextXMLStringValue(psGeneral, "gml:name",
                                    &(map->name));
    }
  }
  /* Keyword */
  if( nVersion >= OWS_1_0_0 ) {
    msLoadMapContextListInMetadata(
      CPLGetXMLNode(psGeneral, "KeywordList"),
      &(map->web.metadata), "KEYWORD", "wms_keywordlist", "," );
  } else
    msGetMapContextXMLHashValue(psGeneral, "Keywords",
                                &(map->web.metadata), "wms_keywordlist");

  /* Window */
  pszValue1 = (char*)CPLGetXMLValue(psGeneral,"Window.width",NULL);
  pszValue2 = (char*)CPLGetXMLValue(psGeneral,"Window.height",NULL);
  if(pszValue1 != NULL && pszValue2 != NULL) {
    map->width = atoi(pszValue1);
    map->height = atoi(pszValue2);
  }

  /* Abstract */
  if( msGetMapContextXMLHashValue( psGeneral,
                                   "Abstract", &(map->web.metadata),
                                   "wms_abstract") == MS_FAILURE ) {
    msGetMapContextXMLHashValue( psGeneral, "gml:description",
                                 &(map->web.metadata), "wms_abstract");
  }

  /* DataURL */
  msGetMapContextXMLHashValueDecode(psGeneral,
                                    "DataURL.OnlineResource.xlink:href",
                                    &(map->web.metadata), "wms_dataurl");

  /* LogoURL */
  /* The logourl have a width, height, format and an URL */
  msLoadMapContextURLELements( CPLGetXMLNode(psGeneral, "LogoURL"),
                               &(map->web.metadata), "wms_logourl" );

  /* DescriptionURL */
  /* The descriptionurl have a width, height, format and an URL */
  msLoadMapContextURLELements( CPLGetXMLNode(psGeneral,
                               "DescriptionURL"),
                               &(map->web.metadata), "wms_descriptionurl" );

  /* Contact Info */
  msLoadMapContextContactInfo(
    CPLGetXMLNode(psGeneral, "ContactInformation"),
    &(map->web.metadata) );

  return MS_SUCCESS;
}

/*
** msLoadMapContextLayer
**
** Load a Layer block from a MapContext document
*/
int msLoadMapContextLayer(mapObj *map, CPLXMLNode *psLayer, int nVersion,
                          char *filename, int unique_layer_names)
{
  char *pszValue;
  const char *pszHash;
  char *pszName=NULL;
  CPLXMLNode *psFormatList, *psFormat, *psStyleList, *psStyle, *psExtension;
  CPLXMLNode *psDimensionList, *psDimension;
  int nStyle;
  layerObj *layer;

  /* Init new layer */
  if(msGrowMapLayers(map) == NULL)
    return MS_FAILURE;

  layer = (GET_LAYER(map, map->numlayers));
  initLayer(layer, map);
  layer->map = (mapObj *)map;
  layer->type = MS_LAYER_RASTER;
  /* save the index */
  GET_LAYER(map, map->numlayers)->index = map->numlayers;
  map->layerorder[map->numlayers] = map->numlayers;
  map->numlayers++;


  /* Status */
  pszValue = (char*)CPLGetXMLValue(psLayer, "hidden", "1");
  if((pszValue != NULL) && (atoi(pszValue) == 0 &&
                            strcasecmp(pszValue, "true") != 0))
    layer->status = MS_ON;
  else
    layer->status = MS_OFF;

  /* Queryable */
  pszValue = (char*)CPLGetXMLValue(psLayer, "queryable", "0");
  if(pszValue !=NULL && (atoi(pszValue) == 1  ||
                         strcasecmp(pszValue, "true") == 0))
    layer->template = msStrdup("ttt");

  /* Name and Title */
  pszValue = (char*)CPLGetXMLValue(psLayer, "Name", NULL);
  if(pszValue != NULL) {
    msInsertHashTable( &(layer->metadata), "wms_name", pszValue );

    if (unique_layer_names) {
      pszName = (char*)malloc(sizeof(char)*(strlen(pszValue)+15));
      sprintf(pszName, "l%d:%s", layer->index, pszValue);
      layer->name = msStrdup(pszName);
      free(pszName);
    } else
      layer->name  = msStrdup(pszValue);
  } else {
    pszName = (char*)malloc(sizeof(char)*15);
    sprintf(pszName, "l%d:", layer->index);
    layer->name = msStrdup(pszName);
    free(pszName);
  }

  if(msGetMapContextXMLHashValue(psLayer, "Title", &(layer->metadata),
                                 "wms_title") == MS_FAILURE) {
    if(msGetMapContextXMLHashValue(psLayer, "Server.title",
                                   &(layer->metadata), "wms_title") == MS_FAILURE) {
      msDebug("Mandatory data Layer.Title missing in %s.", filename);
    }
  }

  /* Server Title */
  msGetMapContextXMLHashValue(psLayer, "Server.title", &(layer->metadata),  "wms_server_title");

  /* Abstract */
  msGetMapContextXMLHashValue(psLayer, "Abstract", &(layer->metadata),
                              "wms_abstract");

  /* DataURL */
  if(nVersion <= OWS_0_1_4) {
    msGetMapContextXMLHashValueDecode(psLayer,
                                      "DataURL.OnlineResource.xlink:href",
                                      &(layer->metadata), "wms_dataurl");
  } else {
    /* The DataURL have a width, height, format and an URL */
    /* Width and height are not used, but they are included to */
    /* be consistent with the spec. */
    msLoadMapContextURLELements( CPLGetXMLNode(psLayer, "DataURL"),
                                 &(layer->metadata), "wms_dataurl" );
  }

  /* The MetadataURL have a width, height, format and an URL */
  /* Width and height are not used, but they are included to */
  /* be consistent with the spec. */
  msLoadMapContextURLELements( CPLGetXMLNode(psLayer, "MetadataURL"),
                               &(layer->metadata), "wms_metadataurl" );


  /* MinScale && MaxScale */
  pszValue = (char*)CPLGetXMLValue(psLayer, "sld:MinScaleDenominator", NULL);
  if(pszValue != NULL) {
    layer->minscaledenom = atof(pszValue);
  }

  pszValue = (char*)CPLGetXMLValue(psLayer, "sld:MaxScaleDenominator", NULL);
  if(pszValue != NULL) {
    layer->maxscaledenom = atof(pszValue);
  }

  /*  */
  /* Server */
  /*  */
  if(nVersion >= OWS_0_1_4) {
    if(msGetMapContextXMLStringValueDecode(psLayer,
                                           "Server.OnlineResource.xlink:href",
                                           &(layer->connection)) == MS_FAILURE) {
      msSetError(MS_MAPCONTEXTERR,
                 "Mandatory data Server.OnlineResource.xlink:href missing in %s.",
                 "msLoadMapContext()", filename);
      return MS_FAILURE;
    } else {
      msGetMapContextXMLHashValueDecode(psLayer,
                                        "Server.OnlineResource.xlink:href",
                                        &(layer->metadata), "wms_onlineresource");
      layer->connectiontype = MS_WMS;
    }
  } else {
    if(msGetMapContextXMLStringValueDecode(psLayer,
                                           "Server.onlineResource",
                                           &(layer->connection)) == MS_FAILURE) {
      msSetError(MS_MAPCONTEXTERR,
                 "Mandatory data Server.onlineResource missing in %s.",
                 "msLoadMapContext()", filename);
      return MS_FAILURE;
    } else {
      msGetMapContextXMLHashValueDecode(psLayer, "Server.onlineResource",
                                        &(layer->metadata), "wms_onlineresource");
      layer->connectiontype = MS_WMS;
    }
  }

  if(nVersion >= OWS_0_1_4) {
    if(msGetMapContextXMLHashValue(psLayer, "Server.version",
                                   &(layer->metadata), "wms_server_version") == MS_FAILURE) {
      msSetError(MS_MAPCONTEXTERR,
                 "Mandatory data Server.version missing in %s.",
                 "msLoadMapContext()", filename);
      return MS_FAILURE;
    }
  } else {
    if(msGetMapContextXMLHashValue(psLayer, "Server.wmtver",
                                   &(layer->metadata), "wms_server_version") == MS_FAILURE) {
      msSetError(MS_MAPCONTEXTERR,
                 "Mandatory data Server.wmtver missing in %s.",
                 "msLoadMapContext()", filename);
      return MS_FAILURE;
    }
  }

  /* Projection */
  msLoadMapContextListInMetadata( psLayer, &(layer->metadata),
                                  "SRS", "wms_srs", " " );

  pszHash = msLookupHashTable(&(layer->metadata), "wms_srs");
  if(((pszHash == NULL) || (strcasecmp(pszHash, "") == 0)) &&
      map->projection.numargs != 0) {
    char* pszProj = map->projection.args[map->projection.numargs-1];

    if(pszProj != NULL) {
      if(strncasecmp(pszProj, "AUTO:", 5) == 0) {
        msInsertHashTable(&(layer->metadata),"wms_srs", pszProj);
      } else {
        if(strlen(pszProj) > 10) {
          pszProj = (char*) malloc(sizeof(char) * (strlen(pszProj)));
          sprintf( pszProj, "EPSG:%s",
                   map->projection.args[map->projection.numargs-1]+10);
          msInsertHashTable(&(layer->metadata),"wms_srs", pszProj);
        } else {
          msDebug("Unable to set data for layer wms_srs from this"
                  " value %s.",
                  pszProj);
        }
      }
      msFree(pszProj);
    }
  }

  /*  */
  /* Format */
  /*  */
  if( nVersion >= OWS_0_1_4 ) {
    psFormatList = CPLGetXMLNode(psLayer, "FormatList");
  } else {
    psFormatList = psLayer;
  }

  if(psFormatList != NULL) {
    for(psFormat = psFormatList->psChild;
        psFormat != NULL;
        psFormat = psFormat->psNext) {
      msLoadMapContextLayerFormat(psFormat, layer);
    }

  } /* end FormatList parsing */

  /* Style */
  if( nVersion >= OWS_0_1_4 ) {
    psStyleList = CPLGetXMLNode(psLayer, "StyleList");
  } else {
    psStyleList = psLayer;
  }

  if(psStyleList != NULL) {
    nStyle = 0;
    for(psStyle = psStyleList->psChild;
        psStyle != NULL;
        psStyle = psStyle->psNext) {
      if(strcasecmp(psStyle->pszValue, "Style") == 0) {
        nStyle++;
        msLoadMapContextLayerStyle(psStyle, layer, nStyle);
      }
    }
  }

  /* Dimension */
  psDimensionList = CPLGetXMLNode(psLayer, "DimensionList");
  if(psDimensionList != NULL) {
    for(psDimension = psDimensionList->psChild;
        psDimension != NULL;
        psDimension = psDimension->psNext) {
      if(strcasecmp(psDimension->pszValue, "Dimension") == 0) {
        msLoadMapContextLayerDimension(psDimension, layer);
      }
    }
  }

  /* Extension */
  psExtension = CPLGetXMLNode(psLayer, "Extension");
  if (psExtension != NULL) {
    pszValue = (char*)CPLGetXMLValue(psExtension, "ol:opacity", NULL);
    if(pszValue != NULL) {
      if(!layer->compositer) {
        layer->compositer = msSmallMalloc(sizeof(LayerCompositer));
        initLayerCompositer(layer->compositer);
      }
      layer->compositer->opacity = atof(pszValue)*100;
    }
  }

  return MS_SUCCESS;
}

#endif



/* msLoadMapContextURL()
**
** load an OGC Web Map Context format from an URL
**
** Take a map object and a URL to a conect file in arguments
*/

int msLoadMapContextURL(mapObj *map, char *urlfilename, int unique_layer_names)
{
#if defined(USE_WMS_LYR)
  char *pszTmpFile = NULL;
  int status = 0;

  if (!map || !urlfilename) {
    msSetError(MS_MAPCONTEXTERR,
               "Invalid map or url given.",
               "msGetMapContextURL()");
    return MS_FAILURE;
  }

  pszTmpFile = msTmpFile(map, map->mappath, NULL, "context.xml");
  if (msHTTPGetFile(urlfilename, pszTmpFile, &status,-1, 0, 0, 0) ==  MS_SUCCESS) {
    return msLoadMapContext(map, pszTmpFile, unique_layer_names);
  } else {
    msSetError(MS_MAPCONTEXTERR,
               "Could not open context file %s.",
               "msGetMapContextURL()", urlfilename);
    return MS_FAILURE;
  }

#else
  msSetError(MS_MAPCONTEXTERR,
             "Not implemented since Map Context is not enabled.",
             "msGetMapContextURL()");
  return MS_FAILURE;
#endif
}
/* msLoadMapContext()
**
** Get a mapfile from a OGC Web Map Context format
**
** Take as first map object and a file in arguments
** If The 2nd aregument unique_layer_names is set to MS_TRUE, the layer
** name created would be unique and be prefixed with an l plus the layers's index
** (eg l:1:park. l:2:road ...). If It is set to MS_FALSE, the layer name
** would be the same name as the layer name in the context
*/
int msLoadMapContext(mapObj *map, char *filename, int unique_layer_names)
{
#if defined(USE_WMS_LYR)
  char *pszWholeText, *pszValue;
  CPLXMLNode *psRoot, *psMapContext, *psLayer, *psLayerList, *psChild;
  char szPath[MS_MAXPATHLEN];
  int nVersion=-1;
  char szVersionBuf[OWS_VERSION_MAXLEN];

  /*  */
  /* Load the raw XML file */
  /*  */

  pszWholeText = msGetMapContextFileText(
                   msBuildPath(szPath, map->mappath, filename));
  if(pszWholeText == NULL) {
    msSetError( MS_MAPCONTEXTERR, "Unable to read %s",
                "msLoadMapContext()", filename );
    return MS_FAILURE;
  }

  if( ( strstr( pszWholeText, "<WMS_Viewer_Context" ) == NULL ) &&
      ( strstr( pszWholeText, "<View_Context" ) == NULL ) &&
      ( strstr( pszWholeText, "<ViewContext" ) == NULL ) )

  {
    free( pszWholeText );
    msSetError( MS_MAPCONTEXTERR, "Not a Map Context file (%s)",
                "msLoadMapContext()", filename );
    return MS_FAILURE;
  }

  /*  */
  /* Convert to XML parse tree. */
  /*  */
  psRoot = CPLParseXMLString( pszWholeText );
  free( pszWholeText );

  /* We assume parser will report errors via CPL. */
  if( psRoot == NULL ) {
    msSetError( MS_MAPCONTEXTERR, "Invalid XML file (%s)",
                "msLoadMapContext()", filename );
    return MS_FAILURE;
  }

  /*  */
  /* Valid the MapContext file and get the root of the document */
  /*  */
  psChild = psRoot;
  psMapContext = NULL;
  while( psChild != NULL ) {
    if( psChild->eType == CXT_Element &&
        (EQUAL(psChild->pszValue,"WMS_Viewer_Context") ||
         EQUAL(psChild->pszValue,"View_Context") ||
         EQUAL(psChild->pszValue,"ViewContext")) ) {
      psMapContext = psChild;
      break;
    } else {
      psChild = psChild->psNext;
    }
  }

  if( psMapContext == NULL ) {
    CPLDestroyXMLNode(psRoot);
    msSetError( MS_MAPCONTEXTERR, "Invalid Map Context File (%s)",
                "msLoadMapContext()", filename );
    return MS_FAILURE;
  }

  /* Fetch document version number */
  pszValue = (char*)CPLGetXMLValue(psMapContext,
                                   "version", NULL);
  if( !pszValue ) {
    msDebug( "msLoadMapContext(): Mandatory data version missing in %s, assuming 0.1.4.",
             filename );
    pszValue = "0.1.4";
  }

  nVersion = msOWSParseVersionString(pszValue);

  /* Make sure this is a supported version */
  switch (nVersion) {
    case OWS_0_1_2:
    case OWS_0_1_4:
    case OWS_0_1_7:
    case OWS_1_0_0:
    case OWS_1_1_0:
      /* All is good, this is a supported version. */
      break;
    default:
      /* Not a supported version */
      msSetError(MS_MAPCONTEXTERR,
                 "This version of Map Context is not supported (%s).",
                 "msLoadMapContext()", pszValue);
      CPLDestroyXMLNode(psRoot);
      return MS_FAILURE;
  }

  /* Reformat and save Version in metadata */
  msInsertHashTable( &(map->web.metadata), "wms_context_version",
                     msOWSGetVersionString(nVersion, szVersionBuf));

  if( nVersion >= OWS_0_1_7 && nVersion < OWS_1_0_0) {
    if( msGetMapContextXMLHashValue(psMapContext, "fid",
                                    &(map->web.metadata), "wms_context_fid") == MS_FAILURE ) {
      msDebug("Mandatory data fid missing in %s.", filename);
    }
  }

  /*  */
  /* Load the General bloc */
  /*  */
  psChild = CPLGetXMLNode( psMapContext, "General" );
  if( psChild == NULL ) {
    CPLDestroyXMLNode(psRoot);
    msSetError(MS_MAPCONTEXTERR,
               "The Map Context document provided (%s) does not contain any "
               "General elements.",
               "msLoadMapContext()", filename);
    return MS_FAILURE;
  }

  if( msLoadMapContextGeneral(map, psChild, psMapContext,
                              nVersion, filename) == MS_FAILURE ) {
    CPLDestroyXMLNode(psRoot);
    return MS_FAILURE;
  }

  /*  */
  /* Load the bloc LayerList */
  /*  */
  psLayerList = CPLGetXMLNode(psMapContext, "LayerList");
  if( psLayerList != NULL ) {
    for(psLayer = psLayerList->psChild;
        psLayer != NULL;
        psLayer = psLayer->psNext) {
      if(EQUAL(psLayer->pszValue, "Layer")) {
        if( msLoadMapContextLayer(map, psLayer, nVersion,
                                  filename, unique_layer_names) == MS_FAILURE ) {
          CPLDestroyXMLNode(psRoot);
          return MS_FAILURE;
        }
      }/* end Layer parsing */
    }/* for */
  }

  CPLDestroyXMLNode(psRoot);

  return MS_SUCCESS;

#else
  msSetError(MS_MAPCONTEXTERR,
             "Not implemented since Map Context is not enabled.",
             "msGetMapContext()");
  return MS_FAILURE;
#endif
}


/* msSaveMapContext()
**
** Save a mapfile into the OGC Web Map Context format
**
** Take a map object and a file in arguments
*/
int msSaveMapContext(mapObj *map, char *filename)
{
#if defined(USE_WMS_LYR)
  VSILFILE *stream;
  char szPath[MS_MAXPATHLEN];
  int nStatus;

  /* open file */
  if(filename != NULL && strlen(filename) > 0) {
    stream = fopen(msBuildPath(szPath, map->mappath, filename), "wb");
    if(!stream) {
      msSetError(MS_IOERR, "(%s)", "msSaveMapContext()", filename);
      return(MS_FAILURE);
    }
  } else {
    msSetError(MS_IOERR, "Filename is undefined.", "msSaveMapContext()");
    return MS_FAILURE;
  }

  nStatus = msWriteMapContext(map, stream);

  fclose(stream);

  return nStatus;
#else
  msSetError(MS_MAPCONTEXTERR,
             "Not implemented since Map Context is not enabled.",
             "msSaveMapContext()");
  return MS_FAILURE;
#endif
}



int msWriteMapContext(mapObj *map, FILE *stream)
{
#if defined(USE_WMS_LYR)
  const char * version;
  char *pszEPSG;
  char * tabspace=NULL, *pszChar,*pszSLD=NULL,*pszSLD2=NULL;
  char *pszStyleItem, *pszSLDBody;
  char *pszEncodedVal;
  int i, nValue, nVersion=OWS_VERSION_NOTSET;
  /* Dimension element */
  char *pszDimension;
  const char *pszDimUserValue=NULL, *pszDimUnits=NULL, *pszDimDefault=NULL;
  const char *pszDimNearValue=NULL, *pszDimUnitSymbol=NULL;
  const char *pszDimMultiValue=NULL;
  int bDimensionList = 0;

  /* Decide which version we're going to return... */
  version = msLookupHashTable(&(map->web.metadata), "wms_context_version");
  if(version == NULL)
    version = "1.1.0";

  nVersion = msOWSParseVersionString(version);
  if (nVersion == OWS_VERSION_BADFORMAT)
    return MS_FAILURE;  /* msSetError() already called. */

  /* Make sure this is a supported version */
  /* Note that we don't write 0.1.2 even if we read it. */
  switch (nVersion) {
    case OWS_0_1_4:
    case OWS_0_1_7:
    case OWS_1_0_0:
    case OWS_1_1_0:
      /* All is good, this is a supported version. */
      break;
    default:
      /* Not a supported version */
      msSetError(MS_MAPCONTEXTERR,
                 "This version of Map Context is not supported (%s).",
                 "msSaveMapContext()", version);
      return MS_FAILURE;
  }

  /* file header */
  msIO_fprintf( stream, "<?xml version='1.0' encoding=\"UTF-8\" standalone=\"no\" ?>\n");

  /* set the WMS_Viewer_Context information */
  pszEncodedVal = msEncodeHTMLEntities(version);
  if(nVersion >= OWS_1_0_0) {
    msIO_fprintf( stream, "<ViewContext version=\"%s\"", pszEncodedVal );
  } else if(nVersion >= OWS_0_1_7) {
    msIO_fprintf( stream, "<View_Context version=\"%s\"", pszEncodedVal );

  } else { /* 0.1.4 */
    msIO_fprintf( stream, "<WMS_Viewer_Context version=\"%s\"", pszEncodedVal );
  }
  msFree(pszEncodedVal);

  if ( nVersion >= OWS_0_1_7 && nVersion < OWS_1_0_0 ) {
    msOWSPrintEncodeMetadata(stream, &(map->web.metadata), NULL,
                             "wms_context_fid", OWS_NOERR," fid=\"%s\"","0");
  }
  if ( nVersion >= OWS_1_0_0 )
    msOWSPrintEncodeParam(stream, "MAP.NAME", map->name, OWS_NOERR,
                          " id=\"%s\"", NULL);

  msIO_fprintf( stream, " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"");
  msIO_fprintf( stream, " xmlns:ogc=\"http://www.opengis.net/ogc\"");

  if( nVersion >= OWS_0_1_7 && nVersion < OWS_1_0_0 ) {
    msIO_fprintf( stream, " xmlns:gml=\"http://www.opengis.net/gml\"");
  }
  if( nVersion >= OWS_1_0_0 ) {
    msIO_fprintf( stream, " xmlns:xlink=\"http://www.w3.org/1999/xlink\"");
    msIO_fprintf( stream, " xmlns=\"http://www.opengis.net/context\"");
    msIO_fprintf( stream, " xmlns:sld=\"http://www.opengis.net/sld\"");
    pszEncodedVal = msEncodeHTMLEntities(msOWSGetSchemasLocation(map));
    if( nVersion >= OWS_1_1_0 )
      msIO_fprintf( stream,
                    " xsi:schemaLocation=\"http://www.opengis.net/context %s/context/1.1.0/context.xsd\">\n",
                    pszEncodedVal);
    else
      msIO_fprintf( stream,
                    " xsi:schemaLocation=\"http://www.opengis.net/context %s/context/1.0.0/context.xsd\">\n",
                    pszEncodedVal);
    msFree(pszEncodedVal);
  } else {
    msIO_fprintf( stream, " xmlns:xlink=\"http://www.w3.org/TR/xlink\"");

    pszEncodedVal = msEncodeHTMLEntities(msOWSGetSchemasLocation(map));
    msIO_fprintf( stream, " xsi:noNamespaceSchemaLocation=\"%s/contexts/",
                  pszEncodedVal );
    msFree(pszEncodedVal);
    pszEncodedVal = msEncodeHTMLEntities(msOWSGetSchemasLocation(map));
    msIO_fprintf( stream, "%s/context.xsd\">\n", pszEncodedVal);
    msFree(pszEncodedVal);
  }

  /* set the General information */
  msIO_fprintf( stream, "  <General>\n" );

  /* Window */
  if( map->width != -1 || map->height != -1 )
    msIO_fprintf( stream, "    <Window width=\"%d\" height=\"%d\"/>\n",
                  map->width, map->height );

  /* Bounding box corners and spatial reference system */
  if(tabspace)
    free(tabspace);
  tabspace = msStrdup("    ");
  msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), "MO", MS_TRUE, &pszEPSG);
  msIO_fprintf( stream,
                "%s<!-- Bounding box corners and spatial reference system -->\n",
                tabspace );
  if(!pszEPSG || (strcasecmp(pszEPSG, "(null)") == 0))
    msIO_fprintf(stream, "<!-- WARNING: Mandatory data 'projection' was missing in this context. -->\n");

  pszEncodedVal = msEncodeHTMLEntities(pszEPSG);
  msIO_fprintf( stream, "%s<BoundingBox SRS=\"%s\" minx=\"%f\" miny=\"%f\" maxx=\"%f\" maxy=\"%f\"/>\n",
                tabspace, pszEncodedVal, map->extent.minx, map->extent.miny,
                map->extent.maxx, map->extent.maxy );
  msFree(pszEncodedVal);
  msFree(pszEPSG);

  /* Title, name */
  if( nVersion >= OWS_0_1_7 && nVersion < OWS_1_0_0 ) {
    msOWSPrintEncodeParam(stream, "MAP.NAME", map->name, OWS_NOERR,
                          "    <gml:name>%s</gml:name>\n", NULL);
  } else {
    if (nVersion < OWS_0_1_7)
      msOWSPrintEncodeParam(stream, "MAP.NAME", map->name, OWS_NOERR,
                            "    <Name>%s</Name>\n", NULL);

    msIO_fprintf( stream, "%s<!-- Title of Context -->\n", tabspace );
    msOWSPrintEncodeMetadata(stream, &(map->web.metadata),
                             NULL, "wms_title", OWS_WARN,
                             "    <Title>%s</Title>\n", map->name);
  }

  /* keyword */
  if (nVersion >= OWS_1_0_0) {
    if (msLookupHashTable(&(map->web.metadata),"wms_keywordlist")!=NULL) {
      char **papszKeywords;
      int nKeywords, iKey;

      const char* pszValue = msLookupHashTable(&(map->web.metadata),
                                   "wms_keywordlist");
      papszKeywords = msStringSplit(pszValue, ',', &nKeywords);
      if(nKeywords > 0 && papszKeywords) {
        msIO_fprintf( stream, "    <KeywordList>\n");
        for(iKey=0; iKey<nKeywords; iKey++) {
          pszEncodedVal = msEncodeHTMLEntities(papszKeywords[iKey]);
          msIO_fprintf( stream, "      <Keyword>%s</Keyword>\n",
                        pszEncodedVal);
          msFree(pszEncodedVal);
        }
        msIO_fprintf( stream, "    </KeywordList>\n");
      }
    }
  } else
    msOWSPrintEncodeMetadataList(stream, &(map->web.metadata), NULL,
                                 "wms_keywordlist",
                                 "    <Keywords>\n", "    </Keywords>\n",
                                 "      %s\n", NULL);

  /* abstract */
  if( nVersion >= OWS_0_1_7 && nVersion < OWS_1_0_0 ) {
    msOWSPrintEncodeMetadata(stream, &(map->web.metadata),
                             NULL, "wms_abstract", OWS_NOERR,
                             "    <gml:description>%s</gml:description>\n", NULL);
  } else {
    msOWSPrintEncodeMetadata(stream, &(map->web.metadata),
                             NULL, "wms_abstract", OWS_NOERR,
                             "    <Abstract>%s</Abstract>\n", NULL);
  }

  /* LogoURL */
  /* The LogoURL have a width, height, format and an URL */
  msOWSPrintURLType(stream, &(map->web.metadata), "MO", "logourl",
                    OWS_NOERR, NULL, "LogoURL", NULL, " width=\"%s\"",
                    " height=\"%s\""," format=\"%s\"",
                    "      <OnlineResource xlink:type=\"simple\" xlink:href=\"%s\"/>\n",
                    MS_FALSE, MS_FALSE, MS_FALSE, MS_FALSE, MS_TRUE,
                    NULL, NULL, NULL, NULL, NULL, "    ");

  /* DataURL */
  msOWSPrintEncodeMetadata(stream, &(map->web.metadata),
                           NULL, "wms_dataurl", OWS_NOERR,
                           "    <DataURL>\n      <OnlineResource xlink:type=\"simple\" xlink:href=\"%s\"/>\n    </DataURL>\n", NULL);

  /* DescriptionURL */
  /* The DescriptionURL have a width, height, format and an URL */
  /* The metadata is structured like this: "width height format url" */
  msOWSPrintURLType(stream, &(map->web.metadata), "MO", "descriptionurl",
                    OWS_NOERR, NULL, "DescriptionURL", NULL, " width=\"%s\"",
                    " height=\"%s\""," format=\"%s\"",
                    "      <OnlineResource xlink:type=\"simple\" xlink:href=\"%s\"/>\n",
                    MS_FALSE, MS_FALSE, MS_FALSE, MS_FALSE, MS_TRUE,
                    NULL, NULL, NULL, NULL, NULL, "    ");

  /* Contact Info */
  msOWSPrintContactInfo( stream, tabspace, OWS_1_1_0, &(map->web.metadata), "MO" );

  /* Close General */
  msIO_fprintf( stream, "  </General>\n" );
  free(tabspace);

  /* Set the layer list */
  msIO_fprintf(stream, "  <LayerList>\n");

  /* Loop on all layer   */
  for(i=0; i<map->numlayers; i++) {
    if(GET_LAYER(map, i)->status != MS_DELETE && GET_LAYER(map, i)->connectiontype == MS_WMS) {
      const char* pszValue;
      char* pszValueMod;
      const char* pszCurrent;
      if(GET_LAYER(map, i)->status == MS_OFF)
        nValue = 1;
      else
        nValue = 0;
      msIO_fprintf(stream, "    <Layer queryable=\"%d\" hidden=\"%d\">\n",
                   msIsLayerQueryable(GET_LAYER(map, i)), nValue);

      /*  */
      /* Server definition */
      /*  */
      if(nVersion < OWS_1_0_0 )
        msOWSPrintEncodeMetadata(stream, &(GET_LAYER(map, i)->metadata),
                                 NULL, "wms_server_version", OWS_WARN,
                                 "      <Server service=\"WMS\" version=\"%s\" ",
                                 "1.0.0");
      else
        msOWSPrintEncodeMetadata(stream, &(GET_LAYER(map, i)->metadata),
                                 NULL, "wms_server_version", OWS_WARN,
                                 "      <Server service=\"OGC:WMS\" version=\"%s\" ",
                                 "1.0.0");

      if(msOWSLookupMetadata(&(GET_LAYER(map, i)->metadata), "MO", "server_title"))
        msOWSPrintEncodeMetadata(stream, &(GET_LAYER(map, i)->metadata),
                                 NULL, "wms_server_title", OWS_NOERR,
                                 "title=\"%s\">\n", "");

      else if(GET_LAYER(map, i)->name)
        msOWSPrintEncodeMetadata(stream, &(GET_LAYER(map, i)->metadata),
                                 NULL, "wms_title", OWS_NOERR,
                                 "title=\"%s\">\n", GET_LAYER(map, i)->name);
      else {
        msOWSPrintEncodeMetadata(stream, &(GET_LAYER(map, i)->metadata),
                                 NULL, "wms_title", OWS_NOERR,
                                 "title=\"%s\">\n", "");
      }

      /* Get base url of the online resource to be the default value */
      if(GET_LAYER(map, i)->connection)
        pszValueMod = msStrdup( GET_LAYER(map, i)->connection );
      else
        pszValueMod = msStrdup( "" );
      pszChar = strchr(pszValueMod, '?');
      if( pszChar )
        pszValueMod[pszChar - pszValueMod] = '\0';
      if(msOWSPrintEncodeMetadata(stream, &(GET_LAYER(map, i)->metadata),
                                  NULL, "wms_onlineresource", OWS_WARN,
                                  "        <OnlineResource xlink:type=\"simple\" xlink:href=\"%s\"/>\n",
                                  pszValueMod) == OWS_WARN)
        msIO_fprintf(stream, "<!-- wms_onlineresource not set, using base URL"
                     " , but probably not what you want -->\n");
      msIO_fprintf(stream, "      </Server>\n");
      if(pszValueMod)
        free(pszValueMod);

      /*  */
      /* Layer information */
      /*  */
      msOWSPrintEncodeMetadata(stream, &(GET_LAYER(map, i)->metadata),
                               NULL, "wms_name", OWS_WARN,
                               "      <Name>%s</Name>\n",
                               GET_LAYER(map, i)->name);
      msOWSPrintEncodeMetadata(stream, &(GET_LAYER(map, i)->metadata),
                               NULL, "wms_title", OWS_WARN,
                               "      <Title>%s</Title>\n",
                               GET_LAYER(map, i)->name);
      msOWSPrintEncodeMetadata(stream, &(GET_LAYER(map, i)->metadata),
                               NULL, "wms_abstract", OWS_NOERR,
                               "      <Abstract>%s</Abstract>\n",
                               NULL);

      /* DataURL */
      if(nVersion <= OWS_0_1_4) {
        msOWSPrintEncodeMetadata(stream, &(GET_LAYER(map, i)->metadata),
                                 NULL, "wms_dataurl", OWS_NOERR,
                                 "      <DataURL>%s</DataURL>\n",
                                 NULL);
      } else {
        /* The DataURL have a width, height, format and an URL */
        /* The metadata will be structured like this:  */
        /* "width height format url" */
        /* Note: in version 0.1.7 the width and height are not included  */
        /* in the Context file, but they are included in the metadata for */
        /* for consistency with the URLType. */
        msOWSPrintURLType(stream, &(GET_LAYER(map, i)->metadata), "MO",
                          "dataurl", OWS_NOERR, NULL, "DataURL", NULL,
                          " width=\"%s\"", " height=\"%s\"",
                          " format=\"%s\"",
                          "        <OnlineResource xlink:type=\"simple\""
                          " xlink:href=\"%s\"/>\n",
                          MS_FALSE, MS_FALSE, MS_FALSE, MS_FALSE,
                          MS_TRUE, NULL, NULL, NULL,NULL,NULL, "      ");
      }

      /* MetadataURL */
      /* The MetadataURL have a width, height, format and an URL */
      /* The metadata will be structured like this:  */
      /* "width height format url" */
      msOWSPrintURLType(stream, &(GET_LAYER(map, i)->metadata), "MO",
                        "metadataurl", OWS_NOERR, NULL, "MetadataURL",NULL,
                        " width=\"%s\"", " height=\"%s\""," format=\"%s\"",
                        "        <OnlineResource xlink:type=\"simple\""
                        " xlink:href=\"%s\"/>\n",
                        MS_FALSE, MS_FALSE, MS_FALSE, MS_FALSE,
                        MS_TRUE, NULL, NULL, NULL, NULL, NULL, "      ");

      /* MinScale && MaxScale */
      if(nVersion >= OWS_1_1_0 && GET_LAYER(map, i)->minscaledenom > 0)
        msIO_fprintf(stream,
                     "      <sld:MinScaleDenominator>%g</sld:MinScaleDenominator>\n",
                     GET_LAYER(map, i)->minscaledenom);
      if(nVersion >= OWS_1_1_0 && GET_LAYER(map, i)->maxscaledenom > 0)
        msIO_fprintf(stream,
                     "      <sld:MaxScaleDenominator>%g</sld:MaxScaleDenominator>\n",
                     GET_LAYER(map, i)->maxscaledenom);

      /* Layer SRS */
      msOWSGetEPSGProj(&(GET_LAYER(map, i)->projection),
                                         &(GET_LAYER(map, i)->metadata),
                                         "MO", MS_FALSE, &pszEPSG);
      if(pszEPSG && (strcasecmp(pszEPSG, "(null)") != 0)) {
        pszEncodedVal = msEncodeHTMLEntities(pszEPSG);
        msIO_fprintf(stream, "      <SRS>%s</SRS>\n", pszEncodedVal);
        msFree(pszEncodedVal);
      }
      msFree(pszEPSG);

      /* Format */
      if(msLookupHashTable(&(GET_LAYER(map, i)->metadata),"wms_formatlist")==NULL &&
          msLookupHashTable(&(GET_LAYER(map, i)->metadata),"wms_format")==NULL) {
        char* pszURL;
        if(GET_LAYER(map, i)->connection)
          pszURL = msStrdup( GET_LAYER(map, i)->connection );
        else
          pszURL = msStrdup( "" );

        pszValueMod = strstr( pszURL, "FORMAT=" );
        if( pszValueMod ) {
          pszValueMod += 7;
          pszChar = strchr(pszValueMod, '&');
          if( pszChar )
            pszValueMod[pszChar - pszValueMod] = '\0';
          if(strcasecmp(pszValueMod, "") != 0) {
            pszEncodedVal = msEncodeHTMLEntities(pszValueMod);
            msIO_fprintf( stream, "      <FormatList>\n");
            msIO_fprintf(stream,"        <Format>%s</Format>\n",pszValueMod);
            msIO_fprintf( stream, "      </FormatList>\n");
            msFree(pszEncodedVal);
          }
        }
        free(pszURL);
      } else {
        char **papszFormats;
        int numFormats, nForm;

        pszValue = msLookupHashTable(&(GET_LAYER(map, i)->metadata),
                                     "wms_formatlist");
        if(!pszValue)
          pszValue = msLookupHashTable(&(GET_LAYER(map, i)->metadata),
                                       "wms_format");
        pszCurrent = msLookupHashTable(&(GET_LAYER(map, i)->metadata),
                                       "wms_format");

        papszFormats = msStringSplit(pszValue, ',', &numFormats);
        if(numFormats > 0 && papszFormats) {
          msIO_fprintf( stream, "      <FormatList>\n");
          for(nForm=0; nForm<numFormats; nForm++) {
            pszEncodedVal =msEncodeHTMLEntities(papszFormats[nForm]);
            if(pszCurrent && (strcasecmp(papszFormats[nForm],
                                         pszCurrent) == 0))
              msIO_fprintf( stream,
                            "        <Format current=\"1\">%s</Format>\n",
                            pszEncodedVal);
            else
              msIO_fprintf( stream, "        <Format>%s</Format>\n",
                            pszEncodedVal);
            msFree(pszEncodedVal);
          }
          msIO_fprintf( stream, "      </FormatList>\n");
        }
        if(papszFormats)
          msFreeCharArray(papszFormats, numFormats);
      }
      /* Style */
      /* First check the stylelist */
      pszValue = msLookupHashTable(&(GET_LAYER(map, i)->metadata),
                                   "wms_stylelist");
      while( pszValue && *pszValue == ' ' )
          pszValue ++;
      if(pszValue == NULL || strlen(pszValue) < 1) {
        /* Check if the style is in the connection URL */
        char* pszURL;
        if(GET_LAYER(map, i)->connection)
          pszURL = msStrdup( GET_LAYER(map, i)->connection );
        else
          pszURL = msStrdup( "" );

        /* Grab the STYLES in the URL */
        pszValueMod = strstr( pszURL, "STYLES=" );
        if( pszValueMod ) {
          pszValueMod += 7;
          pszChar = strchr(pszValueMod, '&');
          if( pszChar )
            pszValueMod[pszChar - pszValueMod] = '\0';

          /* Check the SLD string from the URL */
          if(GET_LAYER(map, i)->connection)
            pszSLD2 = msStrdup(GET_LAYER(map, i)->connection);
          else
            pszSLD2 = msStrdup( "" );
          if(pszSLD2) {
            pszSLD = strstr(pszSLD2, "SLD=");
            pszSLDBody = strstr(pszSLD2, "SLD_BODY=");
          } else {
            pszSLD = NULL;
            pszSLDBody = NULL;
          }
          /* Check SLD */
          if( pszSLD ) {
            pszChar = strchr(pszSLD, '&');
            if( pszChar )
              pszSLD[pszChar - pszSLD] = '\0';
            pszSLD += 4;
          }
          /* Check SLDBody  */
          if( pszSLDBody ) {
            pszChar = strchr(pszSLDBody, '&');
            if( pszChar )
              pszSLDBody[pszChar - pszSLDBody] = '\0';
            pszSLDBody += 9;
          }
          if( (pszValueMod && (strcasecmp(pszValueMod, "") != 0)) ||
              (pszSLD && (strcasecmp(pszSLD, "") != 0)) ||
              (pszSLDBody && (strcasecmp(pszSLDBody, "") != 0))) {
            /* Write Name and Title */
            msIO_fprintf( stream, "      <StyleList>\n");
            msIO_fprintf( stream, "        <Style current=\"1\">\n");
            if( pszValueMod && (strcasecmp(pszValueMod, "") != 0)) {
              pszEncodedVal = msEncodeHTMLEntities(pszValueMod);
              msIO_fprintf(stream, "          <Name>%s</Name>\n",
                           pszEncodedVal);
              msIO_fprintf(stream,"          <Title>%s</Title>\n",
                           pszEncodedVal);
              msFree(pszEncodedVal);
            }
            /* Write the SLD string from the URL */
            if( pszSLD && (strcasecmp(pszSLD, "") != 0)) {
              pszEncodedVal = msEncodeHTMLEntities(pszSLD);
              msIO_fprintf( stream, "          <SLD>\n" );
              msIO_fprintf( stream,
                            "            <OnlineResource xlink:type=\"simple\" ");
              msIO_fprintf(stream,"xlink:href=\"%s\"/>",
                           pszEncodedVal);
              msIO_fprintf( stream, "          </SLD>\n" );
              free(pszEncodedVal);
            } else if(pszSLDBody && (strcasecmp(pszSLDBody, "") != 0)) {
              msIO_fprintf( stream, "          <SLD>\n" );
              msIO_fprintf( stream, "            %s\n",pszSLDBody);
              msIO_fprintf( stream, "          </SLD>\n" );
            }
            msIO_fprintf( stream, "        </Style>\n");
            msIO_fprintf( stream, "      </StyleList>\n");
          }
          if(pszSLD2) {
            free(pszSLD2);
            pszSLD2 = NULL;
          }
        }
        free(pszURL);
      } else {
        const char* pszCurrent;
        /* If the style information is not in the connection URL, */
        /* read the metadata. */
        pszValue = msLookupHashTable(&(GET_LAYER(map, i)->metadata),
                                     "wms_stylelist");
        pszCurrent = msLookupHashTable(&(GET_LAYER(map, i)->metadata),
                                       "wms_style");
        msIO_fprintf( stream, "      <StyleList>\n");
        /* Loop in each style in the style list */
        while(pszValue != NULL) {
          char* pszStyle = msStrdup(pszValue);
          pszChar = strchr(pszStyle, ',');
          if(pszChar != NULL)
            pszStyle[pszChar - pszStyle] = '\0';
          if(pszStyle[0] == '\0')
          {
            msFree(pszStyle);
            continue;
          }

          if(pszCurrent && (strcasecmp(pszStyle, pszCurrent) == 0))
            msIO_fprintf( stream,"        <Style current=\"1\">\n" );
          else
            msIO_fprintf( stream, "        <Style>\n" );

          /* Write SLDURL if it is in the metadata */
          pszStyleItem = (char*)malloc(strlen(pszStyle)+10+10);
          sprintf(pszStyleItem, "wms_style_%s_sld", pszStyle);
          if(msLookupHashTable(&(GET_LAYER(map, i)->metadata),
                               pszStyleItem) != NULL) {
            msIO_fprintf(stream, "          <SLD>\n");
            msOWSPrintEncodeMetadata(stream,
                                     &(GET_LAYER(map, i)->metadata),
                                     NULL, pszStyleItem,
                                     OWS_NOERR,
                                     "            <OnlineResource xlink:type=\"simple\" xlink:href=\"%s\"/>\n",
                                     NULL);
            msIO_fprintf(stream, "          </SLD>\n");
            free(pszStyleItem);
          } else {
            /* If the URL is not there, check for SLDBody */
            sprintf(pszStyleItem, "wms_style_%s_sld_body", pszStyle);
            if(msLookupHashTable(&(GET_LAYER(map, i)->metadata),
                                 pszStyleItem) != NULL) {
              msIO_fprintf(stream, "          <SLD>\n");
              msOWSPrintMetadata(stream,&(GET_LAYER(map, i)->metadata),
                                 NULL, pszStyleItem, OWS_NOERR,
                                 "            %s\n", NULL);
              msIO_fprintf(stream, "          </SLD>\n");
              free(pszStyleItem);
            } else {
              /* If the SLD is not specified, then write the */
              /* name, Title and LegendURL */
              free(pszStyleItem);
              /* Name */
              pszEncodedVal = msEncodeHTMLEntities(pszStyle);
              msIO_fprintf(stream, "          <Name>%s</Name>\n",
                           pszEncodedVal);
              msFree(pszEncodedVal);
              pszStyleItem = (char*)malloc(strlen(pszStyle)+10+8);
              sprintf(pszStyleItem, "wms_style_%s_title",pszStyle);
              /* Title */
              msOWSPrintEncodeMetadata(stream,
                                       &(GET_LAYER(map, i)->metadata),
                                       NULL, pszStyleItem,
                                       OWS_NOERR,
                                       "          <Title>%s</Title>\n",
                                       NULL);
              free(pszStyleItem);

              /* LegendURL */
              pszStyleItem = (char*)malloc(strlen(pszStyle)+10+20);
              sprintf(pszStyleItem, "style_%s_legendurl",
                      pszStyle);
              msOWSPrintURLType(stream, &(GET_LAYER(map, i)->metadata),
                                "M", pszStyleItem, OWS_NOERR, NULL,
                                "LegendURL", NULL, " width=\"%s\"",
                                " height=\"%s\""," format=\"%s\"",
                                "            <OnlineResource "
                                "xlink:type=\"simple\""
                                " xlink:href=\"%s\"/>\n          ",
                                MS_FALSE, MS_FALSE, MS_FALSE,
                                MS_FALSE, MS_TRUE, NULL, NULL,
                                NULL, NULL, NULL, "          ");
              free(pszStyleItem);
            }
          }

          msIO_fprintf( stream,"        </Style>\n" );

          msFree(pszStyle);
          pszValue = strchr(pszValue, ',');
          if(pszValue)
            pszValue++;
        }
        msIO_fprintf( stream, "      </StyleList>\n");
      }

      /* Dimension element */;
      pszCurrent = NULL;

      pszValue = msLookupHashTable(&(GET_LAYER(map, i)->metadata),
                                   "wms_dimensionlist");
      pszCurrent = msLookupHashTable(&(GET_LAYER(map, i)->metadata),
                                     "wms_dimension");
      while(pszValue != NULL) {
        /* Extract the dimension name from the list */
        pszDimension = msStrdup(pszValue);
        pszChar = strchr(pszDimension, ',');
        if(pszChar != NULL)
          pszDimension[pszChar - pszDimension] = '\0';
        if(strcasecmp(pszDimension, "") == 0) {
          free(pszDimension);
          pszValue = strchr(pszValue, ',');
          if(pszValue)
            pszValue++;
          continue;
        }

        /* From the dimension list, extract the required dimension */
        msOWSGetDimensionInfo(GET_LAYER(map, i), pszDimension,
                              &pszDimUserValue, &pszDimUnits,
                              &pszDimDefault, &pszDimNearValue,
                              &pszDimUnitSymbol, &pszDimMultiValue);

        if(pszDimUserValue == NULL || pszDimUnits == NULL ||
            pszDimUnitSymbol == NULL) {
          free(pszDimension);
          pszValue = strchr(pszValue, ',');
          if(pszValue)
            pszValue++;
          continue;
        }

        if(!bDimensionList) {
          bDimensionList = 1;
          msIO_fprintf( stream, "      <DimensionList>\n");
        }

        /* name */
        msIO_fprintf( stream, "        <Dimension name=\"%s\"",
                      pszDimension);
        /* units */
        msIO_fprintf( stream, " units=\"%s\"",      pszDimUnits);
        /* unitSymbol */
        msIO_fprintf( stream, " unitSymbol=\"%s\"", pszDimUnitSymbol);
        /* userValue */
        msIO_fprintf( stream, " userValue=\"%s\"",  pszDimUserValue);
        /* default */
        if(pszDimDefault)
          msIO_fprintf( stream, " default=\"%s\"", pszDimDefault);
        /* multipleValues */
        if(pszDimMultiValue)
          msIO_fprintf(stream, " multipleValues=\"%s\"",
                       pszDimMultiValue);
        /* nearestValue */
        if(pszDimNearValue)
          msIO_fprintf( stream," nearestValue=\"%s\"",pszDimNearValue);

        if(pszCurrent && strcasecmp(pszDimension, pszCurrent) == 0)
          msIO_fprintf( stream, " current=\"1\"");

        msIO_fprintf( stream, "/>\n");

        free(pszDimension);
        pszValue = strchr(pszValue, ',');
        if(pszValue)
          pszValue++;
      }

      if(bDimensionList) {
        msIO_fprintf( stream, "      </DimensionList>\n");
        bDimensionList = 0;
      }

      msIO_fprintf(stream, "    </Layer>\n");
    }
  }

  /* Close layer list */
  msIO_fprintf(stream, "  </LayerList>\n");
  /* Close Map Context */

  if(nVersion >= OWS_1_0_0) {
    msIO_fprintf(stream, "</ViewContext>\n");
  } else if(nVersion >= OWS_0_1_7) {
    msIO_fprintf(stream, "</View_Context>\n");
  } else { /* 0.1.4 */
    msIO_fprintf(stream, "</WMS_Viewer_Context>\n");
  }

  return MS_SUCCESS;
#else
  msSetError(MS_MAPCONTEXTERR,
             "Not implemented since Map Context is not enabled.",
             "msWriteMapContext()");
  return MS_FAILURE;
#endif
}



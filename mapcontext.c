/**********************************************************************
 * $Id$
 *
 * Name:     mapcontext.c
 * Project:  MapServer
 * Language: C
 * Purpose:  OGC Web Map Context implementation
 * Author:   Julien-Samuel Lacroix, DM Solutions Group (lacroix@dmsolutions.ca)
 *
 **********************************************************************
 * Copyright (c) 2002, Julien-Samuel Lacroix, DM Solutions Group Inc
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
 * $Log$
 * Revision 1.7  2002/11/04 20:35:17  julien
 * Change a validation to prevent a crash
 *
 * Revision 1.6  2002/11/04 20:16:09  julien
 * Change a validation to prevent a crash
 *
 * Revision 1.5  2002/10/31 21:25:55  julien
 * transform EPSG:*** to init=epsg:***
 *
 * Revision 1.4  2002/10/31 20:00:07  julien
 * transform EPSG:*** to init=epsg:***
 *
 * Revision 1.3  2002/10/31 18:57:35  sacha
 * Fill layerorder and layer.index in msLoadMapContext
 *
 * Revision 1.2  2002/10/28 22:31:09  dan
 * Added map arg to initLayer() call
 *
 * Revision 1.1  2002/10/28 20:31:20  dan
 * New support for WMS Map Context (from Julien)
 *
 * Revision 1.1  2002/10/22 20:03:57  julien
 * Add the mapcontext support
 *
 **********************************************************************/

#include "map.h"

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
  FILE *stream;
  int	 nLength;

  // open file
  if(filename != NULL && strlen(filename) > 0) {
      stream = fopen(filename, "r");
      if(!stream) {
          msSetError(MS_IOERR, "(%s)", "msGetMapContextFileText()", filename);
          return NULL;
      }
  }
  else
  {
      msSetError(MS_IOERR, "(%s)", "msGetMapContextFileText()", filename);
      return NULL;
  }

  fseek( stream, 0, SEEK_END );
  nLength = ftell( stream );
  fseek( stream, 0, SEEK_SET );

  pszBuffer = (char *) malloc(nLength+1);
  if( pszBuffer == NULL )
  {
      msSetError(MS_MEMERR, "(%s)", "msGetMapContextFileText()", filename);
      fclose( stream );
      return NULL;
  }
    
  if( fread( pszBuffer, nLength, 1, stream ) != 1 )
  {
      free( pszBuffer );
      fclose( stream );
      msSetError(MS_IOERR, "(%s)", "msGetMapContextFileText()", filename);
      return NULL;
  }
  pszBuffer[nLength] = '\0';

  fclose( stream );

  return pszBuffer;
}


/* msGetMapContext()
**
** Get a mapfile from a OGC Web Map Context format
**
** Take a map object and a file in arguments
*/
int msLoadMapContext(mapObj *map, char *filename)
{
#if defined(USE_WMS_LYR)
  char *pszWholeText;
  char *pszValue, *pszValue1, *pszValue2, *pszValue3, *pszValue4;
  char *pszHash, *pszStyle=NULL, *pszStyleName;
  CPLXMLNode *psRoot, *psContactInfo, *psMapContext, *psLayer, *psLayerList;
  CPLXMLNode *psFormatList, *psFormat, *psStyleList, *psStyle;
  char szPath[MS_MAXPATHLEN];
  char szProj[20];
  int nStyle;
  layerObj *layer;

  //
  // Load the raw XML file
  //
  pszWholeText = msGetMapContextFileText(
      msBuildPath(szPath, map->mappath, filename));
  if(pszWholeText == NULL)
  {
      return MS_FAILURE;
  }

  if( strstr( pszWholeText, "<WMS_Viewer_Context" ) == NULL )
  {
      free( pszWholeText );
      msSetError(MS_MAPCONTEXTERR, "(%s)", "msLoadMapContext()", filename);
      return MS_FAILURE;
  }

  //
  // Convert to XML parse tree.
  //
  psRoot = CPLParseXMLString( pszWholeText );
  free( pszWholeText );

  // We assume parser will report errors via CPL.
  if( psRoot == NULL && psRoot->psNext != NULL )
  {
      msSetError(MS_MAPCONTEXTERR, "(%s)", "msLoadMapContext()", filename);
      return MS_FAILURE;
  }

  if( psRoot->psNext->eType != CXT_Element 
      || !EQUAL(psRoot->psNext->pszValue,"WMS_Viewer_Context") )
  {
      CPLDestroyXMLNode(psRoot);
      msSetError(MS_MAPCONTEXTERR, "(%s)", "msLoadMapContext()", filename);
      return MS_FAILURE;
  }
  psMapContext = psRoot->psNext;

  // Schema Location
  pszValue = (char*)CPLGetXMLValue(psMapContext,
                                   "xsi:noNamespaceSchemaLocation",NULL);
  if(pszValue != NULL)
      msInsertHashTable(map->web.metadata,"wfs_schemas_location", pszValue);
  else
  {
      CPLDestroyXMLNode(psRoot);
      msSetError(MS_MAPCONTEXTERR, 
                 "Mandatory data xsi:noNamespaceSchemaLocation missing in %s.",
                 "msLoadMapContext()", filename);
      return MS_FAILURE;
  }

  // Projection
  pszValue = (char*)CPLGetXMLValue(psMapContext, 
                                   "General.BoundingBox.SRS", NULL);
  if(pszValue != NULL)
  {
      sprintf(szProj, "init=epsg:%s", pszValue+5);

      msInitProjection(&map->projection);
      map->projection.args[map->projection.numargs] = strdup(szProj);
      map->projection.numargs++;
      msProcessProjection(&map->projection);
  }
  else
  {
      CPLDestroyXMLNode(psRoot);
      msSetError(MS_MAPCONTEXTERR, 
                 "Mandatory data General.BoundingBox.SRS missing in %s.",
                 "msLoadMapContext()", filename);
      return MS_FAILURE;
  }

  // Extent
  pszValue = (char*)CPLGetXMLValue(psMapContext, 
                                   "General.BoundingBox.minx", NULL);
  if(pszValue != NULL)
      map->extent.minx = atof(pszValue);
  else
  {
      CPLDestroyXMLNode(psRoot);
      msSetError(MS_MAPCONTEXTERR, 
                 "Mandatory data General.BoundingBox.minx missing in %s.",
                 "msLoadMapContext()", filename);
      return MS_FAILURE;
  }
  pszValue = (char*)CPLGetXMLValue(psMapContext, 
                                   "General.BoundingBox.miny", NULL);
  if(pszValue != NULL)
      map->extent.miny = atof(pszValue);
  else
  {
      CPLDestroyXMLNode(psRoot);
      msSetError(MS_MAPCONTEXTERR, 
                 "Mandatory data General.BoundingBox.miny missing in %s.",
                 "msLoadMapContext()", filename);
      return MS_FAILURE;
  }
  pszValue = (char*)CPLGetXMLValue(psMapContext, 
                                   "General.BoundingBox.maxx", NULL);
  if(pszValue != NULL)
      map->extent.maxx = atof(pszValue);
  else
  {
      CPLDestroyXMLNode(psRoot);
      msSetError(MS_MAPCONTEXTERR, 
                 "Mandatory data General.BoundingBox.maxx missing in %s.",
                 "msLoadMapContext()", filename);
      return MS_FAILURE;
  }
  pszValue = (char*)CPLGetXMLValue(psMapContext, 
                                   "General.BoundingBox.maxy", NULL);
  if(pszValue != NULL)
      map->extent.maxy = atof(pszValue);
  else
  {
      CPLDestroyXMLNode(psRoot);
      msSetError(MS_MAPCONTEXTERR, 
                 "Mandatory data General.BoundingBox.maxy missing in %s.",
                 "msLoadMapContext()", filename);
      return MS_FAILURE;
  }

  // Title
  pszValue = (char*)CPLGetXMLValue(psMapContext,"General.Title",NULL);
  if(pszValue != NULL)
      msInsertHashTable(map->web.metadata, "wms_title", pszValue);
  else
  {
      CPLDestroyXMLNode(psRoot);
      msSetError(MS_MAPCONTEXTERR, 
                 "Mandatory data General.Title missing in %s.",
                 "msLoadMapContext()", filename);
      return MS_FAILURE;
  }

  // Name
  pszValue = (char*)CPLGetXMLValue(psMapContext,"General.Name",NULL);
  if(pszValue != NULL)
      map->name = strdup(pszValue);

  // Keyword
  pszValue = (char*)CPLGetXMLValue(psMapContext,"General.Keyword",NULL);
  if(pszValue != NULL)
      msInsertHashTable(map->web.metadata, "wms_keywordlist", pszValue);

  // Window
  pszValue1 = (char*)CPLGetXMLValue(psMapContext,"General.Window.width",NULL);
  pszValue2 = (char*)CPLGetXMLValue(psMapContext,"General.Window.height",NULL);
  if(pszValue != NULL && pszValue != NULL)
  {
      map->width = atoi(pszValue1);
      map->height = atoi(pszValue2);
  }

  // Abstract
  pszValue = (char*)CPLGetXMLValue(psMapContext,"General.Abstract",NULL);
  if(pszValue != NULL)
      msInsertHashTable(map->web.metadata, "wms_abstract", pszValue);

  // Contact Info
  psContactInfo = CPLGetXMLNode(psMapContext, "General.ContactInformation");

  // Contact Person primary
  pszValue = (char*)CPLGetXMLValue(psContactInfo,
                                   "ContactPersonPrimary.ContactPerson",NULL);
  if(pszValue != NULL)
      msInsertHashTable(map->web.metadata, "wms_contactperson", pszValue);
  pszValue = (char*)CPLGetXMLValue(psContactInfo,
                              "ContactPersonPrimary.ContactOrganisation",NULL);
  if(pszValue != NULL)
      msInsertHashTable(map->web.metadata,"wms_contactorganisation", pszValue);

  // Contact Position
  pszValue = (char*)CPLGetXMLValue(psContactInfo,"ContactPosition",NULL);
  if(pszValue != NULL)
      msInsertHashTable(map->web.metadata, "wms_contactposition", pszValue);

  // Contact Address
  pszValue = (char*)CPLGetXMLValue(psContactInfo,
                                   "ContactAddress.AddressType",NULL);
  if(pszValue != NULL)
      msInsertHashTable(map->web.metadata, "wms_addresstype", pszValue);
  pszValue = (char*)CPLGetXMLValue(psContactInfo,
                                   "ContactAddress.Address",NULL);
  if(pszValue != NULL)
      msInsertHashTable(map->web.metadata, "wms_address", pszValue);
  pszValue = (char*)CPLGetXMLValue(psContactInfo,"ContactAddress.City",NULL);
  if(pszValue != NULL)
      msInsertHashTable(map->web.metadata, "wms_city", pszValue);
  pszValue = (char*)CPLGetXMLValue(psContactInfo,
                                   "ContactAddress.StateOrProvince",NULL);
  if(pszValue != NULL)
      msInsertHashTable(map->web.metadata, "wms_stateorprovince", pszValue);
  pszValue = (char*)CPLGetXMLValue(psContactInfo,
                                   "ContactAddress.PostCode",NULL);
  if(pszValue != NULL)
      msInsertHashTable(map->web.metadata, "wms_postcode", pszValue);
  pszValue = (char*)CPLGetXMLValue(psContactInfo,
                                   "ContactAddress.Country",NULL);
  if(pszValue != NULL)
      msInsertHashTable(map->web.metadata, "wms_country", pszValue);

  // Others
  pszValue = (char*)CPLGetXMLValue(psContactInfo,"ContactVoiceTelephone",NULL);
  if(pszValue != NULL)
      msInsertHashTable(map->web.metadata, "wms_contactvoicetelephone", 
                        pszValue);
  pszValue = (char*)CPLGetXMLValue(psContactInfo,
                                   "ContactFascimileTelephone",NULL);
  if(pszValue != NULL)
      msInsertHashTable(map->web.metadata, 
                        "wms_contactfascimiletelephone", pszValue);
  pszValue = (char*)CPLGetXMLValue(psContactInfo,
                                   "ContactElectronicMailAddress",NULL);
  if(pszValue != NULL)
      msInsertHashTable(map->web.metadata, 
                        "wms_contactelectronicmailaddress", pszValue);

  // LayerList
  psLayerList = CPLGetXMLNode(psMapContext, "LayerList");
  if( psLayerList != NULL )
  {
      for(psLayer = psLayerList->psChild; 
          psLayer != NULL; 
          psLayer = psLayer->psNext)
      {
          if(EQUAL(psLayer->pszValue, "Layer"))
          {
              // Init new layer
              layer = &(map->layers[map->numlayers]);
              initLayer(layer, map);
              layer->map = (mapObj *)map;
              /* save the index */
              map->layers[map->numlayers].index = map->numlayers;
              map->layerorder[map->numlayers] = map->numlayers;
              map->numlayers++;
              if(layer->metadata == NULL)
                  layer->metadata =  msCreateHashTable();

              // Status
              pszValue = (char*)CPLGetXMLValue(psLayer, "hidden", "0");
              if((pszValue != NULL) && (atoi(pszValue) == 1))
                  layer->status = MS_ON;
              else
                  layer->status = MS_OFF;

              // Queryable
              pszValue = (char*)CPLGetXMLValue(psLayer, "queryable", "0");
              if(pszValue !=NULL && atoi(pszValue) == 1)
                  layer->template = strdup("ttt");

              // Others
              pszValue =  (char*)CPLGetXMLValue(psLayer, "Name", NULL);
              if(pszValue != NULL)
                  layer->name = strdup(pszValue);
              else
              {
                  CPLDestroyXMLNode(psRoot);
                  msSetError(MS_MAPCONTEXTERR, 
                             "Mandatory data Layer.Name missing in %s.",
                             "msLoadMapContext()", filename);
                  return MS_FAILURE;
              }
              pszValue = (char*)CPLGetXMLValue(psLayer, "Title", NULL);
              if(pszValue != NULL)
                  msInsertHashTable(layer->metadata, "wms_title", pszValue);
              else
              {
                  pszValue=(char*)CPLGetXMLValue(psLayer,"Server.title",NULL);
                  if(pszValue != NULL)
                      msInsertHashTable(layer->metadata,"wms_title",pszValue);
                  else
                  {
                      CPLDestroyXMLNode(psRoot);
                      msSetError(MS_MAPCONTEXTERR, 
                                 "Mandatory data Layer.Title missing in %s.",
                                 "msLoadMapContext()", filename);
                      return MS_FAILURE;
                  }
              }
              pszValue =  (char*)CPLGetXMLValue(psLayer, "Abstract", NULL);
              if(pszValue != NULL)
                  msInsertHashTable(layer->metadata, "wms_abstract", pszValue);

              // Server
              pszValue = (char*)CPLGetXMLValue(psLayer, 
                               "Server.OnlineResource.xlink:href", NULL);
              if(pszValue != NULL)
              {
                  msInsertHashTable(layer->metadata, "wms_onlineresource", 
                                    pszValue);
                  layer->connection = strdup(pszValue);
                  layer->connectiontype = MS_WMS;
              }
              else
              {
                  CPLDestroyXMLNode(psRoot);
                  msSetError(MS_MAPCONTEXTERR, 
              "Mandatory data Server.OnlineResource.xlink:href missing in %s.",
                             "msLoadMapContext()", filename);
                  return MS_FAILURE;
              }
              pszValue =(char*)CPLGetXMLValue(psLayer, "Server.version", NULL);
              if(pszValue != NULL)
                  msInsertHashTable(layer->metadata, "wms_server_version", 
                                    pszValue);
              else
              {
                  CPLDestroyXMLNode(psRoot);
                  msSetError(MS_MAPCONTEXTERR, 
                             "Mandatory data Server.version missing in %s.",
                             "msLoadMapContext()", filename);
                  return MS_FAILURE;
              }

              // Projection
              pszValue = (char*)CPLGetXMLValue(psLayer, "SRS", NULL);
              if(pszValue != NULL)
              {
                  sprintf(szProj, "init=epsg:%s", pszValue+5);

                  msInitProjection(&layer->projection);
                  layer->projection.args[layer->projection.numargs] = 
                      strdup(szProj);
                  layer->projection.numargs++;
                  msProcessProjection(&layer->projection);
              }

              // Format
              psFormatList = CPLGetXMLNode(psLayer, "FormatList");
              if(psFormatList != NULL)
              {
                  for(psFormat = psFormatList->psChild; 
                      psFormat != NULL; 
                      psFormat = psFormat->psNext)
                  {
                      if(psFormat->psChild->psNext == NULL)
                          pszValue = psFormat->psChild->pszValue;
                      else
                          pszValue = psFormat->psChild->psNext->pszValue;
                      if(pszValue != NULL)
                      {
                          // wms_format
                          pszValue1 = (char*)CPLGetXMLValue(psFormat, 
                                                            "current", NULL);
                          if(pszValue1 != NULL)
                              msInsertHashTable(layer->metadata, 
                                                "wms_format", pszValue);
                          // wms_formatlist
                          pszHash = msLookupHashTable(layer->metadata, 
                                                      "wms_formatlist");
                          if(pszHash != NULL)
                          {
                              pszValue1 = (char*)malloc(strlen(pszHash)+
                                                        strlen(pszValue)+2);
                              sprintf(pszValue1, "%s %s", pszHash, pszValue);
                              msInsertHashTable(layer->metadata, 
                                                "wms_formatlist", pszValue1);
                              free(pszValue1);
                          }
                          else
                              msInsertHashTable(layer->metadata, 
                                                "wms_formatlist", pszValue);
                      }
                      else
                      {
                          CPLDestroyXMLNode(psRoot);
                          msSetError(MS_MAPCONTEXTERR, 
                             "Mandatory data FormatList.Format missing in %s.",
                                     "msLoadMapContext()", filename);
                          return MS_FAILURE;
                      }
                  }
              }

              // Style
              psStyleList = CPLGetXMLNode(psLayer, "StyleList");
              if(psStyleList != NULL)
              {
                  nStyle = 0;
                  for(psStyle = psStyleList->psChild; 
                      psStyle != NULL; 
                      psStyle = psStyle->psNext)
                  {
                      pszStyleName =(char*)CPLGetXMLValue(psStyle,"Name",NULL);
                      if(pszStyleName == NULL)
                      {
                          nStyle++;
                          pszStyleName = (char*)malloc(15);
                          sprintf(pszStyleName, "Style{%d}", nStyle);
                      }
                      else
                          pszStyleName = strdup(pszStyleName);

                      // wms_style
                      pszValue = (char*)CPLGetXMLValue(psStyle,"current",NULL);
                      if(pszValue != NULL)
                          msInsertHashTable(layer->metadata, 
                                            "wms_style", pszStyleName);
                      // wms_stylelist
                      pszHash = msLookupHashTable(layer->metadata, 
                                                  "wms_stylelist");
                      if(pszHash != NULL)
                      {
                          pszValue1 = (char*)malloc(strlen(pszHash)+
                                                    strlen(pszStyleName)+2);
                          sprintf(pszValue1, "%s %s", pszHash, pszStyleName);
                          msInsertHashTable(layer->metadata, 
                                            "wms_stylelist", pszValue1);
                          free(pszValue1);
                      }
                      else
                          msInsertHashTable(layer->metadata, 
                                            "wms_stylelist", pszStyleName);

                      // title
                      pszValue = (char*)CPLGetXMLValue(psStyle, "Title", NULL);
                      if(pszValue != NULL)
                      {
                          pszStyle = (char*)malloc(strlen(pszStyleName)+20);
                          sprintf(pszStyle,"wms_style_%s_title",pszStyleName);
                          msInsertHashTable(layer->metadata,pszStyle,pszValue);
                          free(pszStyle);
                      }
                      else
                          msInsertHashTable(layer->metadata, pszStyle, 
                                            layer->name);
                      // SLD
                      pszValue = (char*)CPLGetXMLValue(psStyle, 
                                        "SLD.OnlineResource.xlink:href", NULL);
                      if(pszValue != NULL)
                      {
                          pszStyle = (char*)malloc(strlen(pszStyleName)+15);
                          sprintf(pszStyle, "wms_style_%s_sld", pszStyleName);
                          msInsertHashTable(layer->metadata,pszStyle,pszValue);
                          free(pszStyle);
                      }
                      // LegendURL
                      pszValue=(char*)CPLGetXMLValue(psStyle,
                                   "LegendURL.OnlineResource.xlink:href",NULL);
                      if(pszValue != NULL)
                      {
                          pszStyle = (char*)malloc(strlen(pszStyleName)+25);
                          sprintf(pszStyle, "wms_style_%s_legendurl",
                                  pszStyleName);
                          pszValue1=(char*)CPLGetXMLValue(psStyle,
                                                         "LegendURL.width","");
                          pszValue2=(char*)CPLGetXMLValue(psStyle,
                                                        "LegendURL.height","");
                          pszValue3=(char*)CPLGetXMLValue(psStyle,
                                                        "LegendURL.format","");
                          pszValue4=(char*)CPLGetXMLValue(psStyle, 
                                     "LegendURL.OnlineResource.xlink:href","");
                          pszValue = (char*)malloc(strlen(pszValue1)+
                                                   strlen(pszValue2)+
                                                   strlen(pszValue3)+
                                                   strlen(pszValue4)+6);
                          sprintf(pszValue, "%s %s %s %s", pszValue1,pszValue2,
                                  pszValue3, pszValue4);
                          msInsertHashTable(layer->metadata,pszStyle,pszValue);
                          free(pszStyle);
                          free(pszValue);
                      }

                      free(pszStyleName);
                  }

                  if(msLookupHashTable(layer->metadata, 
                                       "wms_stylelist") == NULL)
                  {
                      pszValue = strdup(layer->connection);
                      pszValue = strstr(pszValue, "STYLELIST=") + 10;
                      pszValue1 = strchr(pszValue, '&');
                      pszValue[pszValue1-pszValue] = '\0';
                      msInsertHashTable(layer->metadata, "wms_stylelist",
                                        pszValue);
                      free(pszValue);
                  }
                  if(msLookupHashTable(layer->metadata, "wms_style") == NULL)
                  {
                      pszValue = strdup(layer->connection);
                      pszValue = strstr(pszValue, "STYLE=") + 6;
                      pszValue1 = strchr(pszValue, '&');
                      pszValue[pszValue1-pszValue] = '\0';
                      msInsertHashTable(layer->metadata, "wms_style",
                                        pszValue);
                      free(pszValue);
                  }
              }
          }
      }
  }

  CPLDestroyXMLNode(psRoot);

  return MS_SUCCESS;

#else
  msSetError(MS_MAPCONTEXTERR, 
             "Not implemented since  Map Context is not enabled.",
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
  const char * version, *schemas_location, *value;
  char * tabspace=NULL, *pszValue, *pszChar,*pszSLD=NULL,*pszURL,*pszSLD2=NULL;
  char *pszFormat, *pszStyle, *pszCurrent, *pszStyleItem, *pszLegendURL;
  char *pszLegendItem;
  FILE *stream;
  int i, nValue;
  char szPath[MS_MAXPATHLEN];

  // Decide which version we're going to return... only 1.0.0 for now
  version = "0.1.4";

  // open file
  if(filename != NULL && strlen(filename) > 0) {
    stream = fopen(msBuildPath(szPath, map->mappath, filename), "wb");
    if(!stream) {
      msSetError(MS_IOERR, "(%s)", "msSaveMapContext()", filename);
      return(MS_FAILURE);
    }
  }
  else
  {
      msSetError(MS_IOERR, "(%s)", "msSaveMapContext()", filename);
      return MS_FAILURE;
  }

  // file header
  fprintf( stream, "<?xml version=\"1.0\" encoding=\"utf-8\'?>\n" );

  // set the WMS_Viewer_Context information
  // Use value of "wfs_schemas_location", otherwise return ".."
  schemas_location = msLookupHashTable(map->web.metadata, 
                                       "wfs_schemas_location");
  if (schemas_location == NULL)
      schemas_location = "..";

  fprintf( stream, "<WMS_Viewer_Context version=\"%s\"", version );
  fprintf( stream, " xmlns:xlink=\"http://www.w3.org/TR/xlink\"" );
  fprintf( stream, " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"");
  fprintf( stream, " xsi:noNamespaceSchemaLcation=\"%s\">\n",schemas_location);

  // set the General information
  fprintf( stream, "  <General>\n" );

  // Bounding box corners and spatial reference system
  if(tabspace)
      free(tabspace);
  tabspace = strdup("    ");
  value = msGetEPSGProj(&(map->projection), map->web.metadata, MS_TRUE);
  fprintf( stream, 
          "%s<!-- Bounding box corners and spatial reference system -->\n", 
           tabspace );
  fprintf( stream, "%s<BoundingBox SRS=\"%s\" minx=\"%g\" miny=\"%g\" maxx=\"%g\" maxy=\"%g\"/>\n", 
           tabspace, value, map->extent.minx, map->extent.miny, 
           map->extent.maxx, map->extent.maxy );

  // Title, Keyword and Abstractof Context
  fprintf( stream, "%s<Name>%s</Name>\n", tabspace, map->name );
  fprintf( stream, "%s<!-- Title of Context -->\n", tabspace );
  msOWSPrintMetadata(stream, map->web.metadata, "wms_title", OWS_WARN,
                "    <Title>%s</Title>\n", map->name);
  msOWSPrintMetadataList(stream, map->web.metadata, "wms_keywordlist", 
                    "    <Keywords>\n", "    </Keywords>\n",
                    "      %s\n");
  msOWSPrintMetadata(stream, map->web.metadata, "wms_abstract", OWS_NOERR,
                "    <Abstract>%s</Abstract>\n", NULL);

  // Window
  fprintf( stream, "    <Window width=%d height=%d/>", 
           map->width, map->height );

  // Contact Info
  msOWSPrintContactInfo( stream, tabspace, "1.1.0", map->web.metadata );
  // Close General
  fprintf( stream, "  </General>\n" );
  free(tabspace);

  // Set the layer list
  fprintf(stream, "  <!-- LayerList of Layer elements\n");
  fprintf(stream, "       - implied order: bottom To Top\n");
  fprintf(stream, 
          "       first Layer element is bottom most layer in map view\n");
  fprintf(stream, "       last Layer elements is op most layer in map view\n");
  fprintf(stream, "  -->\n");
  fprintf(stream, "  <LayerList>\n");

  // Loop on all layer  
  for(i=0; i<map->numlayers; i++)
  {
      if(map->layers[i].connectiontype == MS_WMS)
      {
          if(map->layers[i].status == MS_OFF)
              nValue = 0;
          else
              nValue = 1;
          fprintf(stream, "    <Layer queryable=\"%d\" hidden=\"%d\">\n", 
                  msIsLayerQueryable(&(map->layers[i])), nValue);

          // 
          // Server definition
          //
          fprintf(stream, "      <Server service=\"WMS\" ");
          msOWSPrintMetadata(stream, map->layers[i].metadata, 
                             "wms_server_version", OWS_WARN,"version=\"%s\" ",
                             "1.1.0");
          msOWSPrintMetadata(stream, map->layers[i].metadata, "wms_title",
                             OWS_WARN, "title=\"%s\">\n", map->layers[i].name);
          // Get base url of the online resource to be the default value
          pszValue = strdup( map->layers[i].connection );
          fprintf(stream, "        <OnlineResource xlink:type=\"simple\" ");
          if(msOWSPrintMetadata(stream, map->layers[i].metadata, 
                             "wms_onlineresource", OWS_WARN, 
                             "xlink:href=\"%s\"/>\n", pszValue) == OWS_WARN)
              fprintf(stream, "<!-- wms_onlineresource not set, using base URL , but probably not what you want -->");
          fprintf(stream, "      </Server>\n");
          if(pszValue)
              free(pszValue);

          //
          // Layer information
          //
          fprintf(stream, "      <Name>%s</Name>\n", map->layers[i].name);
          msOWSPrintMetadata(stream, map->layers[i].metadata, "wms_title",
                             OWS_WARN, "      <Title>%s</Title>\n", 
                             map->layers[i].name);
          msOWSPrintMetadata(stream, map->layers[i].metadata, "wms_abstract",
                             OWS_NOERR, "      <Abstract>%s</Abstract>\n", 
                             NULL);
          fprintf(stream, "      <SRS>%s</SRS>\n", 
                  msGetEPSGProj(&(map->layers[i].projection), 
                                map->layers[i].metadata, MS_TRUE));
          // Format
          if(msLookupHashTable(map->layers[i].metadata,"wms_formatlist")==NULL)
          {
              pszURL = strdup( map->layers[i].connection );
              pszValue = pszURL;
              pszValue = strstr( pszValue, "FORMAT=" );
              if( pszValue )
              {
                  pszValue += 7;
                  pszChar = strchr(pszValue, '&');
                  if( pszChar )
                      pszValue[pszChar - pszValue] = '\0';
                  fprintf( stream, "      <FormatList>\n");
                  fprintf( stream, "        <Format>%s</Format>\n", pszValue);
                  fprintf( stream, "      </FormatList>\n");
              }
              if(pszURL)
                  free(pszURL);
          }
          else
          {
              pszValue = msLookupHashTable(map->layers[i].metadata, 
                                           "wms_formatlist");
              pszCurrent = msLookupHashTable(map->layers[i].metadata, 
                                             "wms_format");
              fprintf( stream, "      <FormatList>\n");
              while(pszValue != NULL && pszValue[0] != '\0')
              {
                  pszFormat = strdup(pszValue);
                  pszChar = strchr(pszFormat, ' ');
                  if(pszChar != NULL)
                      pszFormat[pszChar - pszFormat] = '\0';
                  if(strcasecmp(pszFormat, pszCurrent) == 0)
                      fprintf(stream,"        <Format current=1>%s</Format>\n",
                               pszFormat);
                  else
                      fprintf( stream, "        <Format>%s</Format>\n", 
                               pszFormat);
                  free(pszFormat);
                  pszValue = strchr(pszValue, ' ');
                  if(pszValue)
                      pszValue++;
              }
              fprintf( stream, "      </FormatList>\n");
          }

          // Style
          if(msLookupHashTable(map->layers[i].metadata,"wms_stylelist") ==NULL)
          {
              pszURL = strdup( map->layers[i].connection );
              pszValue = pszURL;
              pszValue = strstr( pszValue, "STYLES=" );
              if( pszValue )
              {
                  pszValue += 7;
                  pszChar = strchr(pszValue, '&');
                  if( pszChar )
                      pszValue[pszChar - pszValue] = '\0';

                  pszSLD2 = strdup(map->layers[i].connection);
                  pszSLD = pszSLD2;
                  pszSLD = strstr(pszSLD, "SLD=");
                  if( pszSLD )
                  {
                      pszChar = strchr(pszSLD, '&');
                      if( pszChar )
                          pszSLD[pszChar - pszSLD] = '\0';
                      pszSLD += 4;
                  }
                  if( pszValue || pszSLD )
                  {
                      fprintf( stream, "      <StyleList>\n");
                      fprintf( stream, "        <Style current=\"1\">\n");
                      if( pszValue )
                      {
                          fprintf(stream, "          <Name>%s</Name>\n", 
                                  pszValue);
                          fprintf(stream,"          <Title>%s</Title>\n",
                                  pszValue);
                      }
                      if( pszSLD )
                      {
                          fprintf( stream, "          <SLD>\n" );
                          fprintf( stream, 
                         "            <OnlineResource xlink:type=\"simple\" ");
                          fprintf( stream, "xlink:href=\"%s\"/>", pszSLD );
                          fprintf( stream, "          </SLD>\n" );
                      }
                      fprintf( stream, "        </Style>\n");
                      fprintf( stream, "      </StyleList>\n");
                  }
              }
              if(pszURL)
                  free(pszURL);
              if(pszSLD2)
                  free(pszSLD2);
          }
          else
          {
              pszValue = msLookupHashTable(map->layers[i].metadata, 
                                           "wms_stylelist");
              pszCurrent = msLookupHashTable(map->layers[i].metadata, 
                                             "wms_style");
              fprintf( stream, "      <StyleList>\n");
              while(pszValue != NULL && pszValue[0] != '\0')
              {
                  pszStyle = strdup(pszValue);
                  pszChar = strchr(pszStyle, ' ');
                  if(pszChar != NULL)
                      pszStyle[pszChar - pszStyle] = '\0';
                  if(strcasecmp(pszStyle, pszCurrent) == 0)
                      fprintf( stream,"        <Style current=1>\n" );
                  else
                      fprintf( stream, "        <Style>\n" );

                  pszStyleItem = (char*)malloc(strlen(pszStyle)+10+5);
                  sprintf(pszStyleItem, "wms_style_%s_sld", pszStyle);
                  if(msLookupHashTable(map->layers[i].metadata,
                                       pszStyleItem) != NULL)
                  {
                      fprintf(stream, "          <SLD>\n");
                      msOWSPrintMetadata(stream, map->layers[i].metadata, 
                                         pszStyleItem, OWS_NOERR, 
     "            <OnlineResource xlink:type=\"simple\" xlink:href=\"%s\"/>\n",
                                         NULL);
                      fprintf(stream, "          </SLD>\n");
                      free(pszStyleItem);
                  }
                  else
                  {
                      free(pszStyleItem);
                      fprintf(stream, "          <Name>%s</Name>\n", pszStyle);
                      pszStyleItem = (char*)malloc(strlen(pszStyle)+10+8);
                      sprintf(pszStyleItem, "wms_style_%s_title", pszStyle);
                      msOWSPrintMetadata(stream, map->layers[i].metadata, 
                                         pszStyleItem, OWS_NOERR, 
                                         "          <Title>%s</Title>\n", 
                                         NULL);
                      free(pszStyleItem);

                      pszStyleItem = (char*)malloc(strlen(pszStyle)+10+15);
                      sprintf(pszStyleItem, "wms_style_%s_legendurl",pszStyle);
                      pszLegendURL = msLookupHashTable(map->layers[i].metadata,
                                                       pszStyleItem);
                      if(pszLegendURL != NULL)
                      {
                          fprintf(stream, "          <LegendURL ");
                          // width
                          pszLegendItem = strdup(pszLegendURL);
                          pszChar = strchr(pszLegendItem, ' ');
                          if(pszChar != NULL)
                              pszLegendItem[pszChar-pszLegendItem] = '\0';
                          fprintf(stream, "width=\"%s\" ", pszLegendItem);
                          // height
                          pszLegendURL+=strlen(pszLegendItem)+1;
                          strcpy(pszLegendItem, pszLegendURL);
                          pszChar = strchr(pszLegendItem, ' ');
                          if(pszChar != NULL)
                              pszLegendItem[pszChar-pszLegendItem] = '\0';
                          fprintf(stream, "height=\"%s\" ", pszLegendItem);
                          // format
                          pszLegendURL+=strlen(pszLegendItem)+1;
                          strcpy(pszLegendItem, pszLegendURL);
                          pszChar = strchr(pszLegendItem, ' ');
                          if(pszChar != NULL)
                              pszLegendItem[pszChar-pszLegendItem] = '\0';
                          fprintf(stream, "format=\"%s\">\n", pszLegendItem);
                          // URL
                          pszLegendURL+=strlen(pszLegendItem)+1;
                          strcpy(pszLegendItem, pszLegendURL);
                          pszChar = strchr(pszLegendItem, ' ');
                          if(pszChar != NULL)
                              pszLegendItem[pszChar-pszLegendItem] = '\0';
                          fprintf(stream, 
     "            <OnlineResource xlink:type=\"simple\" xlink:href=\"%s\"/>\n",
                                  pszLegendItem);
                          free(pszLegendItem);

                          fprintf(stream,  "          </LegendURL>\n");
                      }
                      free(pszStyleItem);
                  }

                  fprintf( stream,"        </Style>\n" );
                  free(pszStyle);
                  pszValue = strchr(pszValue, ' ');
                  if(pszValue)  
                      pszValue++;
              }
              fprintf( stream, "      </StyleList>\n");
          }

          fprintf(stream, "    </Layer>\n");
      }
  }

  // Close layer list
  fprintf(stream, "  </LayerList>\n");
  // Close Map Context
  fprintf(stream, "</WMS_Viewer_Context>\n");

  fclose(stream);

  return MS_SUCCESS;
#else
  msSetError(MS_MAPCONTEXTERR, 
             "Not implemented since  Map Context is not enabled.",
             "msSaveMapContext()");
  return MS_FAILURE;
#endif
}


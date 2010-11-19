/******************************************************************************
 * 
 *
 * Project:  MapServer
 * Purpose:  Google Earth KML output
 * Author:   David Kana and the MapServer team
 *
 ******************************************************************************
 * Copyright (c) 1996-2009 Regents of the University of Minnesota.
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
 *****************************************************************************/

#ifdef USE_KML

#include "mapserver.h"
#include "maperror.h"
#include "mapkmlrenderer.h"
#include "mapio.h"


KmlRenderer::KmlRenderer(int width, int height, outputFormatObj *format, colorObj* color/*=NULL*/) 
	:	XmlDoc(NULL), LayerNode(NULL), GroundOverlayNode(NULL), Width(width), Height(height),
		FirstLayer(MS_TRUE), MapCellsize(1.0),
		PlacemarkNode(NULL), GeomNode(NULL),
		Items(NULL), NumItems(0), map(NULL), currentLayer(NULL)

{
    /*private variables*/        
    pszLayerDescMetadata = NULL;  
    papszLayerIncludeItems = NULL;
    nIncludeItems=0;
    
    LineStyle = NULL;
    numLineStyle = 0;

    xmlNodePtr styleNode;
    xmlNodePtr listStyleNode;
  /*	Create document.*/
    XmlDoc = xmlNewDoc(BAD_CAST "1.0");

    xmlNodePtr rootNode = xmlNewNode(NULL, BAD_CAST "kml");

    /*	Name spaces*/
    xmlSetNs(rootNode, xmlNewNs(rootNode, BAD_CAST "http://www.opengis.net/kml/2.2", NULL));

    xmlDocSetRootElement(XmlDoc, rootNode);

	DocNode = xmlNewChild(rootNode, NULL, BAD_CAST "Document", NULL);

        styleNode = xmlNewChild(DocNode, NULL, BAD_CAST "Style", NULL);
	xmlNewProp(styleNode, BAD_CAST "id", BAD_CAST "LayerFolder_check");
	listStyleNode = xmlNewChild(styleNode, NULL, BAD_CAST "ListStyle", NULL);
	xmlNewChild(listStyleNode, NULL, BAD_CAST "listItemType", BAD_CAST "check");

        styleNode = xmlNewChild(DocNode, NULL, BAD_CAST "Style", NULL);
	xmlNewProp(styleNode, BAD_CAST "id", BAD_CAST "LayerFolder_checkHideChildren");
	listStyleNode = xmlNewChild(styleNode, NULL, BAD_CAST "ListStyle", NULL);
	xmlNewChild(listStyleNode, NULL, BAD_CAST "listItemType", BAD_CAST "checkHideChildren");

        styleNode = xmlNewChild(DocNode, NULL, BAD_CAST "Style", NULL);
	xmlNewProp(styleNode, BAD_CAST "id", BAD_CAST "LayerFolder_checkOffOnly");
	listStyleNode = xmlNewChild(styleNode, NULL, BAD_CAST "ListStyle", NULL);
	xmlNewChild(listStyleNode, NULL, BAD_CAST "listItemType", BAD_CAST "checkOffOnly");

        styleNode = xmlNewChild(DocNode, NULL, BAD_CAST "Style", NULL);
	xmlNewProp(styleNode, BAD_CAST "id", BAD_CAST "LayerFolder_radioFolder");
	listStyleNode = xmlNewChild(styleNode, NULL, BAD_CAST "ListStyle", NULL);
	xmlNewChild(listStyleNode, NULL, BAD_CAST "listItemType", BAD_CAST "radioFolder");


	StyleHashTable = msCreateHashTable();
	
}

KmlRenderer::~KmlRenderer()
{
	if (XmlDoc)
		xmlFreeDoc(XmlDoc);

	if (StyleHashTable)
		msFreeHashTable(StyleHashTable);

	xmlCleanupParser();
}

imageObj* KmlRenderer::createImage(int, int, outputFormatObj*, colorObj*)
{
	return NULL;
}

int KmlRenderer::saveImage(imageObj *, FILE *fp, outputFormatObj*)
{
    
    /* -------------------------------------------------------------------- */
    /*      Write out the document.                                         */
    /* -------------------------------------------------------------------- */

    int bufSize = 0;
    xmlChar *buf = NULL;
    msIOContext *context = NULL;

    if( msIO_needBinaryStdout() == MS_FAILURE )
      return MS_FAILURE;

    xmlDocDumpFormatMemoryEnc(XmlDoc, &buf, &bufSize, "UTF-8", 1);

    context = msIO_getHandler(fp);

    int chunkSize = 4096;
    for (int i=0; i<bufSize; i+=chunkSize)
    {
        int size = chunkSize;
        if (i + size > bufSize)
          size = bufSize - i;

        if (context)
          msIO_contextWrite(context, buf+i, size);
        else
          msIO_fwrite(buf+i, 1, size, fp);
    }

    xmlFree(buf);

    return(MS_SUCCESS);
}


/************************************************************************/
/*                               processLayer                           */
/*                                                                      */
/*      Set parameters that make sense to a kml output.                 */
/************************************************************************/
void KmlRenderer::processLayer(layerObj *layer)
{
    int i;
    if (!layer)
      return;

    /*turn of labelcache*/
    layer->labelcache = MS_OFF;
    
    /*if there are labels we want to coordinates to 
      be the center of the element.*/
    for(i=0; i<layer->numclasses; i++)
      layer->_class[i]->label.position = MS_XY;
   
    /*we do not want to draw multiple styles.
      the new rendering architecture does not allow
      to know if we are dealing with a multi-style.
      So here we remove all styles beside the first one*/
    /*
    for(i=0; i<layer->numclasses; i++)
    {
        while (layer->_class[i]->numstyles > 1)
          msDeleteStyle(layer->_class[i], layer->_class[i]->numstyles-1);
    }
    */
}

char* KmlRenderer::getLayerName(layerObj *layer)
{
    char stmp[20];
    const char *name=NULL;;

    if (!layer)
      return NULL;

    if (layer->name && strlen(layer->name) > 0)
      return strdup(layer->name);

     name = msLookupHashTable(&layer->metadata, "ows_name");
     if (name && strlen(name) > 0)
       return strdup(name);

     sprintf(stmp, "Layer%d",layer->index);
     return strdup(stmp); 

}

int KmlRenderer::startNewLayer(imageObj *, layerObj *layer)
{
    char *layerName=NULL;
    const char *value=NULL;

    LayerNode = xmlNewNode(NULL, BAD_CAST "Folder");
	
    layerName = getLayerName(layer);
    xmlNewChild(LayerNode, NULL, BAD_CAST "name", BAD_CAST layerName);
    msFree(layerName);

    const char *layerVisibility = layer->status != MS_OFF ? "1" : "0";
    xmlNewChild(LayerNode, NULL, BAD_CAST "visibility", BAD_CAST layerVisibility);

    const char *layerDsiplayFolder = msLookupHashTable(&(layer->metadata), "kml_folder_display");
    if (layerDsiplayFolder == NULL)
      layerDsiplayFolder = msLookupHashTable(&(layer->map->web.metadata), "kml_folder_display");
    if (!layerDsiplayFolder || strlen(layerDsiplayFolder)<=0)
    {
        xmlNewChild(LayerNode, NULL, BAD_CAST "styleUrl", BAD_CAST "#LayerFolder_check"); 
    }

    else
    {
        if (strcasecmp(layerDsiplayFolder, "checkHideChildren") == 0)
          xmlNewChild(LayerNode, NULL, BAD_CAST "styleUrl", BAD_CAST "#LayerFolder_checkHideChildren");
        else if (strcasecmp(layerDsiplayFolder, "checkOffOnly") == 0)
          xmlNewChild(LayerNode, NULL, BAD_CAST "styleUrl", BAD_CAST "#LayerFolder_checkOffOnly");
        else if (strcasecmp(layerDsiplayFolder, "radioFolder") == 0)
          xmlNewChild(LayerNode, NULL, BAD_CAST "styleUrl", BAD_CAST "#LayerFolder_radioFolder");
        else 
          xmlNewChild(LayerNode, NULL, BAD_CAST "styleUrl", BAD_CAST "#LayerFolder_check");
    }

   

    

    /*Init few things on the first layer*/
    if (FirstLayer)
    {
        FirstLayer = MS_FALSE;
        map = layer->map;

        if (layer->map->mappath)
            snprintf(MapPath, sizeof(MapPath), "%s", layer->map->mappath);

        /*First rendered layer - check mapfile projection*/
        checkProjection(layer->map);

        /*check for image path and image url*/
        if (layer->map->debug && (layer->map->web.imageurl == NULL ||   layer->map->web.imagepath == NULL))
          msDebug("KmlRenderer::startNewLayer: imagepath and imageurl sould be set in the web object\n");


        /*map rect for ground overlay*/
        MapExtent = layer->map->extent;
        MapCellsize = layer->map->cellsize;
        BgColor = layer->map->imagecolor;

         xmlNewChild(DocNode, NULL, BAD_CAST "name", BAD_CAST layer->map->name);
         
         for (int i=0; i<map->numoutputformats; i++)
         {
             outputFormatObj *iFormat = layer->map->outputformatlist[i];
             if(!strcasecmp(iFormat->name,"png24"))
             {
                 aggFormat = iFormat;
                 break;
             }
         } 

        
    }

    currentLayer = layer;

   

     if (!msLayerIsOpen(layer))
     {
         if (msLayerOpen(layer) != MS_SUCCESS)
         {
             msSetError(MS_MISCERR, "msLayerOpen failed", "KmlRenderer::startNewLayer" );
         }
     }

     /*pre process the layer to set things that make sense for kml output*/
     processLayer(layer);

     if (msLookupHashTable(&layer->metadata, "kml_description"))
       pszLayerDescMetadata = msLookupHashTable(&layer->metadata, "kml_description");
     
     value=msLookupHashTable(&layer->metadata, "kml_include_items");
     if (value)
       papszLayerIncludeItems = msStringSplit(value, ',', &nIncludeItems);

     
     DumpAttributes = MS_FALSE;
     char *attribVal = msLookupHashTable(&layer->metadata, "kml_dumpattributes");
     if (attribVal && strlen(attribVal) > 0)
     {

         DumpAttributes = MS_TRUE;
         msLayerWhichItems(layer, MS_FALSE, attribVal);

     }
     else
     {
         msLayerWhichItems(layer, MS_TRUE, NULL);
     }

     NumItems = layer->numitems;
     if (NumItems)
     {
         Items = (char **)calloc(NumItems, sizeof(char *));
         for (int i=0; i<NumItems; i++)
           Items[i] = strdup(layer->items[i]);
     }

    

    setupRenderingParams(&layer->metadata);
    return MS_SUCCESS;
}

int KmlRenderer::closeNewLayer(imageObj *img, layerObj *layer)
{
    flushPlacemark();
        
    xmlAddChild(DocNode, LayerNode);

    if(Items)
    {
        msFreeCharArray(Items, NumItems);
        Items = NULL;
        NumItems = 0;
    }

    if (pszLayerDescMetadata)
      pszLayerDescMetadata = NULL;

    if (papszLayerIncludeItems && nIncludeItems>0)
      msFreeCharArray(papszLayerIncludeItems, nIncludeItems);

    papszLayerIncludeItems=NULL;
    return MS_SUCCESS;
}

int KmlRenderer::mergeRasterBuffer(imageObj *image, rasterBufferObj *rb) {
   assert(rb && rb->type == MS_BUFFER_BYTE_RGBA);
   char *tmpFileName = NULL;
   char *tmpUrl = NULL;
   FILE *tmpFile = NULL;
    
   tmpFileName = msTmpFile(MapPath, image->imagepath, "png");
   tmpFile = fopen(tmpFileName,"w");
   if (tmpFile)
   {
     
     if (!aggFormat->vtable)
      msInitializeRendererVTable(aggFormat);

     msSaveRasterBuffer(rb,tmpFile,aggFormat);
     tmpUrl = strdup( image->imageurl);
     tmpUrl = msStringConcatenate(tmpUrl, (char *)(msGetBasename(tmpFileName)));
     tmpUrl = msStringConcatenate(tmpUrl, ".png");
        
     createGroundOverlayNode(LayerNode, tmpUrl, currentLayer);
     msFree(tmpFileName);
     msFree(tmpUrl);
     fclose(tmpFile);
     return MS_SUCCESS;
   } else {
      msSetError(MS_IOERR,"Failed to create file for kml overlay","KmlRenderer::mergeRasterBuffer()");
      return MS_FAILURE;
   }
}

void KmlRenderer::setupRenderingParams(hashTableObj *layerMetadata)
{
	AltitudeMode = 0;
	Extrude = 0;
	Tessellate = 0;

	char *altitudeModeVal = msLookupHashTable(layerMetadata, "kml_altitudeMode");
	if (altitudeModeVal)
	{
		if(strcasecmp(altitudeModeVal, "absolute") == 0)
			AltitudeMode = absolute;
		else if(strcasecmp(altitudeModeVal, "relativeToGround") == 0)
			AltitudeMode = relativeToGround;
		else if(strcasecmp(altitudeModeVal, "clampToGround") == 0)
			AltitudeMode = clampToGround;
	}

	char *extrudeVal = msLookupHashTable(layerMetadata, "kml_extrude");
	if (altitudeModeVal)
	{
		Extrude = atoi(extrudeVal);
	}

	char *tessellateVal = msLookupHashTable(layerMetadata, "kml_tessellate");
	if (tessellateVal)
	{
		Tessellate = atoi(tessellateVal);
	}

}

int KmlRenderer::checkProjection(mapObj *map)
{
    
    projectionObj *projection= &map->projection;
#ifdef USE_PROJ
    if (projection && projection->numargs > 0 && pj_is_latlong(projection->proj))
    {
        return MS_SUCCESS;
    }
    else
    {
        char epsg_string[100];
        rectObj sRect;
        projectionObj out;
        strcpy(epsg_string, "epsg:4326" );
        msInitProjection(&out);
        msLoadProjectionString(&out, epsg_string);

        sRect = map->extent;
        msProjectRect(projection, &out, &sRect);
        msFreeProjection(projection);
        msLoadProjectionString(projection, epsg_string);

        /*change also units and extents*/
        map->extent = sRect;
        map->units = MS_DD;


        if (map->debug)
          msDebug("KmlRenderer::checkProjection: Mapfile projection set to epsg:4326\n");

        return MS_SUCCESS;
    }

#else
    msSetError(MS_MISCERR, "Projection support not enabled", "KmlRenderer::checkProjection" );
    return MS_FAILURE;
#endif
}

xmlNodePtr KmlRenderer::createPlacemarkNode(xmlNodePtr parentNode, char *styleUrl)
{

    xmlNodePtr placemarkNode = xmlNewChild(parentNode, NULL, BAD_CAST "Placemark", NULL);
    /*always add a name. It will be replaced by a text value if available*/
    char tmpid[100];
    char *stmp=NULL, *layerName=NULL;
    sprintf(tmpid, ".%d_%d", CurrentShapeIndex);
    layerName = getLayerName(currentLayer);
    stmp = msStringConcatenate(stmp, layerName);
    stmp = msStringConcatenate(stmp, tmpid);
    xmlNewChild(placemarkNode, NULL, BAD_CAST "name", BAD_CAST stmp);
    msFree(layerName);
    msFree(stmp);
    if (styleUrl)
      xmlNewChild(placemarkNode, NULL, BAD_CAST "styleUrl", BAD_CAST styleUrl);

    return placemarkNode;
}

void KmlRenderer::renderLine(imageObj*, shapeObj *p, strokeStyleObj *style)
{
    if (p->numlines == 0)
      return;

    if (PlacemarkNode == NULL)
      PlacemarkNode = createPlacemarkNode(LayerNode, NULL);

    if (!PlacemarkNode)
      return;

    addLineStyleToList(style);    
    SymbologyFlag[Line] = 1;

    /*p->index > CurrentDrawnShapeIndexneed to be reviewd. Added since the hight
      level code caches shapes when rendering lines*/
    if (CurrentDrawnShapeIndex == -1 || p->index > CurrentDrawnShapeIndex)
    {
    
      xmlNodePtr geomNode = getGeomParentNode("LineString");
      addAddRenderingSpecifications(geomNode);
      addCoordsNode(geomNode, p->line[0].point, p->line[0].numpoints);
          
          /* more than one line => MultiGeometry*/
      if (p->numlines > 1)
      {
        geomNode = getGeomParentNode("LineString"); // returns MultiGeom Node
        for (int i=1; i<p->numlines; i++)
        {
          xmlNodePtr lineStringNode = xmlNewChild(geomNode, NULL, BAD_CAST "LineString", NULL);
          addAddRenderingSpecifications(lineStringNode);
          addCoordsNode(lineStringNode, p->line[i].point, p->line[i].numpoints);
        }
      }
      
      CurrentDrawnShapeIndex = p->index;
    }
      
}

void KmlRenderer::renderPolygon(imageObj*, shapeObj *p, colorObj *color)
{
    if (PlacemarkNode == NULL)
      PlacemarkNode = createPlacemarkNode(LayerNode, NULL);

    if (!PlacemarkNode)
      return;

    memcpy(&PolygonColor, color, sizeof(colorObj));
    SymbologyFlag[Polygon] = 1;

    
    if (p->index != CurrentDrawnShapeIndex)
    {
    
      xmlNodePtr geomParentNode = getGeomParentNode("Polygon");

      for (int i=0; i<p->numlines; i++)
      {
        xmlNodePtr bdryNode = NULL;

        if (i==0) /* __TODO__ check ring order*/
          bdryNode = xmlNewChild(geomParentNode, NULL, BAD_CAST "outerBoundaryIs", NULL);
        else
          bdryNode = xmlNewChild(geomParentNode, NULL, BAD_CAST "innerBoundaryIs", NULL);

        xmlNodePtr ringNode = xmlNewChild(bdryNode, NULL, BAD_CAST "LinearRing", NULL);
        addAddRenderingSpecifications(ringNode);
        addCoordsNode(ringNode, p->line[i].point, p->line[i].numpoints);
      }

      CurrentDrawnShapeIndex = p->index;
      
    }
      
}

void KmlRenderer::addCoordsNode(xmlNodePtr parentNode, pointObj *pts, int numPts)
{
    char lineBuf[128];

    xmlNodePtr coordsNode = xmlNewChild(parentNode, NULL, BAD_CAST "coordinates", NULL);
    xmlNodeAddContent(coordsNode, BAD_CAST "\n");

    for (int i=0; i<numPts; i++)
    {
      if (AltitudeMode == relativeToGround || AltitudeMode == absolute)
      {
#ifdef USE_POINT_Z_M
        sprintf(lineBuf, "\t%.8f,%.8f,%.8f\n", pts[i].x, pts[i].y, pts[i].z);
#else
        msSetError(MS_MISCERR, "Z coordinates support not available  (mapserver not compiled with USE_POINT_Z_M option)", "KmlRenderer::addCoordsNode()");
#endif
      }
      else
        sprintf(lineBuf, "\t%.8f,%.8f\n", pts[i].x, pts[i].y);

      xmlNodeAddContent(coordsNode, BAD_CAST lineBuf);
    }
    xmlNodeAddContent(coordsNode, BAD_CAST "\t");
}

void KmlRenderer::renderGlyphs(imageObj*, double x, double y, labelStyleObj *style, char *text)
{
    xmlNodePtr node; 

    if (PlacemarkNode == NULL)
      PlacemarkNode = createPlacemarkNode(LayerNode, NULL);

    if (!PlacemarkNode)
      return;

    memcpy(&LabelStyle, style, sizeof(labelStyleObj));
    SymbologyFlag[Label] = 1;

    /*there is alaws a default name (layer.shapeid). Replace it*/
    for (node = PlacemarkNode->children; node; node = node->next) 
    {
        if (node->type != XML_ELEMENT_NODE)
          continue;

        if (strcmp((char *)node->name, "name") == 0)
        {
            xmlNodeSetContent(node,  BAD_CAST text);
            break;
        }
    }
            
    /*xmlNewChild(PlacemarkNode, NULL, BAD_CAST "name", BAD_CAST text);*/

    xmlNodePtr geomNode = getGeomParentNode("Point");
    addAddRenderingSpecifications(geomNode);

    pointObj pt;
    pt.x = x; pt.y = y;
    addCoordsNode(geomNode, &pt, 1);
}

void KmlRenderer::addAddRenderingSpecifications(xmlNodePtr node)
{
	/*
          <extrude>0</extrude>                   <!-- boolean -->
          <tessellate>0</tessellate>             <!-- boolean -->
          <altitudeMode>clampToGround</altitudeMode> 
	*/

    if (Extrude)
      xmlNewChild(node, NULL, BAD_CAST "extrude", BAD_CAST "1");

    if (Tessellate)
      xmlNewChild(node, NULL, BAD_CAST "tessellate", BAD_CAST "1");

    if (AltitudeMode == absolute)
      xmlNewChild(node, NULL, BAD_CAST "altitudeMode", BAD_CAST "absolute");
    else if (AltitudeMode == relativeToGround)
      xmlNewChild(node, NULL, BAD_CAST "altitudeMode", BAD_CAST "relativeToGround");
    else if (AltitudeMode == clampToGround)
      xmlNewChild(node, NULL, BAD_CAST "altitudeMode", BAD_CAST "clampToGround");
}


imageObj *agg2CreateImage(int width, int height, outputFormatObj *format, colorObj * bg);

int KmlRenderer::createIconImage(char *fileName, symbolObj *symbol, symbolStyleObj *symstyle)
{
    pointObj p;
    
    imageObj *tmpImg = NULL;
    
    tmpImg = agg2CreateImage((int)(symbol->sizex*symstyle->scale), 
                            (int)(symbol->sizey*symstyle->scale), 
                            aggFormat, NULL);
    tmpImg->format = aggFormat;
    if (!aggFormat->vtable)
      msInitializeRendererVTable(aggFormat);

    p.x = symbol->sizex * symstyle->scale / 2;
    p.y = symbol->sizey *symstyle->scale / 2;
#ifdef USE_POINT_Z_M
    p.z = 0.0;
#endif

    msDrawMarkerSymbol(&map->symbolset,tmpImg, &p, symstyle->style, 1);

    return msSaveImage(map, tmpImg, fileName);
}

void KmlRenderer::renderSymbol(imageObj *img, double x, double y, symbolObj *symbol, symbolStyleObj *style)
{
    if (PlacemarkNode == NULL)
      PlacemarkNode = createPlacemarkNode(LayerNode, NULL);

    if (!PlacemarkNode)
      return;

    snprintf(SymbolUrl, sizeof(SymbolUrl), "%s", lookupSymbolUrl(img, symbol, style));
    SymbologyFlag[Symbol] = 1;

    xmlNodePtr geomNode = getGeomParentNode("Point");
    addAddRenderingSpecifications(geomNode);

    pointObj pt;
    pt.x = x; pt.y = y;
    addCoordsNode(geomNode, &pt, 1);
}

void KmlRenderer::renderPixmapSymbol(imageObj *img, double x, double y, symbolObj *symbol, 
                                     symbolStyleObj *style)
{
    renderSymbol(img, x, y, symbol, style);
}

void KmlRenderer::renderVectorSymbol(imageObj *img, double x, double y, symbolObj *symbol, 
                                     symbolStyleObj *style)
{
    renderSymbol(img, x, y, symbol, style);
}

void KmlRenderer::renderEllipseSymbol(imageObj *img, double x, double y, symbolObj *symbol, 
                                      symbolStyleObj *style)
{
    renderSymbol(img, x, y, symbol, style);
}

void KmlRenderer::renderTruetypeSymbol(imageObj *img, double x, double y, symbolObj *symbol, 
                                       symbolStyleObj *style)
{
    renderSymbol(img, x, y, symbol, style);
}

xmlNodePtr KmlRenderer::createGroundOverlayNode(xmlNodePtr parentNode, char *imageHref, layerObj *layer)
{
	/*
          <?xml version="1.0" encoding="UTF-8"?>
          <kml xmlns="http://www.opengis.net/kml/2.2">
          <GroundOverlay>
          <name>GroundOverlay.kml</name>
          <color>7fffffff</color>
          <drawOrder>1</drawOrder>
          <Icon>
          <href>http://www.google.com/intl/en/images/logo.gif</href>
          <refreshMode>onInterval</refreshMode>
          <refreshInterval>86400</refreshInterval>
          <viewBoundScale>0.75</viewBoundScale>
          </Icon>
          <LatLonBox>
          <north>37.83234</north>
          <south>37.832122</south>
          <east>-122.373033</east>
          <west>-122.373724</west>
          <rotation>45</rotation>
          </LatLonBox>
          </GroundOverlay>
          </kml>
	*/

    xmlNodePtr groundOverlayNode = xmlNewChild(parentNode, NULL, BAD_CAST "GroundOverlay", NULL);
    char *layerName = getLayerName(layer);
    xmlNewChild(groundOverlayNode, NULL, BAD_CAST "name", BAD_CAST layerName);
    xmlNewChild(groundOverlayNode, NULL, BAD_CAST "color", BAD_CAST "ffffffff");
    char stmp[20];
    sprintf(stmp, "%d",layer->index);
    xmlNewChild(groundOverlayNode, NULL, BAD_CAST "drawOrder", BAD_CAST stmp);

    if (imageHref)
    {
      xmlNodePtr iconNode = xmlNewChild(groundOverlayNode, NULL, BAD_CAST "Icon", NULL);
      xmlNewChild(iconNode, NULL, BAD_CAST "href", BAD_CAST imageHref);
    }

    char crdStr[64];
    xmlNodePtr latLonBoxNode = xmlNewChild(groundOverlayNode, NULL, BAD_CAST "LatLonBox", NULL);
    sprintf(crdStr, "%.8f", currentLayer->map->extent.maxy);
    xmlNewChild(latLonBoxNode, NULL, BAD_CAST "north", BAD_CAST crdStr);

    sprintf(crdStr, "%.8f", currentLayer->map->extent.miny);
    xmlNewChild(latLonBoxNode, NULL, BAD_CAST "south", BAD_CAST crdStr);

    sprintf(crdStr, "%.8f", currentLayer->map->extent.minx);
    xmlNewChild(latLonBoxNode, NULL, BAD_CAST "west", BAD_CAST crdStr);

    sprintf(crdStr, "%.8f", currentLayer->map->extent.maxx);
    xmlNewChild(latLonBoxNode, NULL, BAD_CAST "east", BAD_CAST crdStr);

    xmlNewChild(latLonBoxNode, NULL, BAD_CAST "rotation", BAD_CAST "0.0");

    return groundOverlayNode;
}

void KmlRenderer::startShape(imageObj *, shapeObj *shape)
{
    if (PlacemarkNode)
      flushPlacemark();

    CurrentShapeIndex=-1;
    CurrentDrawnShapeIndex = -1;

    /*should be done at endshape but the plugin architecture does not call endshape yet*/
    if(LineStyle)
    {
        msFree(LineStyle);

        LineStyle = NULL;
        numLineStyle = 0;
    }

    if (shape)
    {
        CurrentShapeIndex = shape->index;
    }
    PlacemarkNode = NULL;
    GeomNode = NULL;

    DescriptionNode = createDescriptionNode(shape);

    memset(SymbologyFlag, 0, NumSymbologyFlag);
}

void KmlRenderer::endShape(imageObj*, shapeObj*)
{
    CurrentShapeIndex = -1;
}

xmlNodePtr KmlRenderer::getGeomParentNode(char *geomName)
{
    
    /*we do not need a multi-geometry for point layers*/
    if (currentLayer->type != MS_LAYER_POINT && currentLayer->type != MS_LAYER_ANNOTATION && GeomNode)
    {
        /*placemark geometry already defined, we need multigeometry node*/
        xmlNodePtr multiGeomNode = xmlNewNode(NULL, BAD_CAST "MultiGeometry");
        xmlAddChild(multiGeomNode, GeomNode);
        GeomNode = multiGeomNode;

        xmlNodePtr geomNode = xmlNewChild(multiGeomNode, NULL, BAD_CAST geomName, NULL);
        return geomNode;
    }
    else
    {
      GeomNode = xmlNewNode(NULL, BAD_CAST geomName);
      return GeomNode;
    }
}

char* KmlRenderer::lookupSymbolUrl(imageObj *img, symbolObj *symbol, symbolStyleObj *style)
{
    char	symbolHexColor[32];
	/*	
                <Style id="randomColorIcon">
                <IconStyle>
                <color>ff00ff00</color>
                <colorMode>random</colorMode>
                <scale>1.1</scale>
                <Icon>
                <href>http://maps.google.com/mapfiles/kml/pal3/icon21.png</href>
                </Icon>
                </IconStyle>
                </Style>
	*/

    sprintf(symbolHexColor,"%02x%02x%02x%02x", style->color->alpha, style->color->blue,
            style->color->green, style->color->red);
    snprintf(SymbolName, sizeof(SymbolName), "symbol_%s_%.1f_%s", symbol->name, style->scale, symbolHexColor);

    char *symbolUrl = msLookupHashTable(StyleHashTable, SymbolName);
    if (!symbolUrl)
    {
      char iconFileName[MS_MAXPATHLEN];
      char iconUrl[MS_MAXPATHLEN];

      if (img->imagepath)
      {
        char *tmpFileName = msTmpFile(MapPath, img->imagepath, "png");
        snprintf(iconFileName, sizeof(iconFileName), "%s", tmpFileName);
        msFree(tmpFileName);
      }
      else
      {
        sprintf(iconFileName, "symbol_%s_%.1f.%s", symbol->name, style->scale, "png");
      }

      if (createIconImage(iconFileName, symbol, style) != MS_SUCCESS)
      {
        char errMsg[512];
        sprintf(errMsg, "Error creating icon file '%s'", iconFileName);
        msSetError(MS_IOERR, errMsg, "KmlRenderer::lookupSymbolStyle()" );
        return NULL;
      }

      if (img->imageurl)
        sprintf(iconUrl, "%s%s.%s", img->imageurl, msGetBasename(iconFileName), "png");
      else
        snprintf(iconUrl, sizeof(iconUrl), "%s", iconFileName);

      hashObj *hash = msInsertHashTable(StyleHashTable, SymbolName, iconUrl);
      symbolUrl = hash->data;
    }

    return symbolUrl;
}

char* KmlRenderer::lookupPlacemarkStyle()
{
    char	lineHexColor[32];
    char	polygonHexColor[32];
    char	labelHexColor[32];
    char        *styleName=NULL;

    styleName = msStringConcatenate(styleName, "style");

    if (SymbologyFlag[Line])
    {
          /*
            <LineStyle id="ID">
            <!-- inherited from ColorStyle -->
            <color>ffffffff</color>            <!-- kml:color -->
            <colorMode>normal</colorMode>      <!-- colorModeEnum: normal or random -->

            <!-- specific to LineStyle -->
            <width>1</width>                   <!-- float -->
            </LineStyle>
          */
    
        for (int i=0; i<numLineStyle; i++)
        {
            if (currentLayer && currentLayer->opacity > 0 && currentLayer->opacity < 100 &&
                LineStyle[i].color->alpha == 255)
              LineStyle[i].color->alpha = MS_NINT(currentLayer->opacity*2.55);
              
            sprintf(lineHexColor,"%02x%02x%02x%02x", LineStyle[i].color->alpha, LineStyle[0].color->blue,
                    LineStyle[i].color->green, LineStyle[i].color->red);

            char lineStyleName[32];
            sprintf(lineStyleName, "_line_%s_w%.1f", lineHexColor, LineStyle[i].width);
            styleName = msStringConcatenate(styleName, lineStyleName);
        }
    }

    if (SymbologyFlag[Polygon])
    {
          /*
            <PolyStyle id="ID">
            <!-- inherited from ColorStyle -->
            <color>ffffffff</color>            <!-- kml:color -->
            <colorMode>normal</colorMode>      <!-- kml:colorModeEnum: normal or random -->

            <!-- specific to PolyStyle -->
            <fill>1</fill>                     <!-- boolean -->
            <outline>1</outline>               <!-- boolean -->
            </PolyStyle>
          */

      if (currentLayer && currentLayer->opacity > 0 && currentLayer->opacity < 100 &&
          PolygonColor.alpha == 255)
        PolygonColor.alpha = MS_NINT(currentLayer->opacity*2.55);
      sprintf(polygonHexColor,"%02x%02x%02x%02x", PolygonColor.alpha, PolygonColor.blue, PolygonColor.green, PolygonColor.red);

      char polygonStyleName[64];
      sprintf(polygonStyleName, "_polygon_%s", polygonHexColor);
      styleName = msStringConcatenate(styleName, polygonStyleName);
    }

    if (SymbologyFlag[Label])
    {
          /*
            <LabelStyle id="ID">
            <!-- inherited from ColorStyle -->
            <color>ffffffff</color>            <!-- kml:color -->
            <colorMode>normal</colorMode>      <!-- kml:colorModeEnum: normal or random -->

            <!-- specific to LabelStyle -->
            <scale>1</scale>                   <!-- float -->
            </LabelStyle>
          */

      if (currentLayer && currentLayer->opacity > 0 && currentLayer->opacity < 100 &&
          LabelStyle.color->alpha == 255)
        LabelStyle.color->alpha = MS_NINT(currentLayer->opacity*2.55);
      sprintf(labelHexColor,"%02x%02x%02x%02x", LabelStyle.color->alpha, LabelStyle.color->blue, LabelStyle.color->green, LabelStyle.color->red);

          // __TODO__ add label scale

      char labelStyleName[64];
      sprintf(labelStyleName, "_label_%s", labelHexColor);
      styleName = msStringConcatenate(styleName, labelStyleName);
    }

    if (SymbologyFlag[Symbol])
    {
          /*	
		<Style id="randomColorIcon">
                <IconStyle>
                <color>ff00ff00</color>
                <colorMode>random</colorMode>
                <scale>1.1</scale>
                <Icon>
                <href>http://maps.google.com/mapfiles/kml/pal3/icon21.png</href>
                </Icon>
                </IconStyle>
		</Style>
          */

          /* __TODO__ add label scale */

      styleName = msStringConcatenate(styleName, "_");
      styleName = msStringConcatenate(styleName, SymbolName);
    }

    char *styleUrl = msLookupHashTable(StyleHashTable, styleName);
    if (!styleUrl)
    {
        char *styleValue=NULL;
        styleValue = msStringConcatenate(styleValue, "#");
        styleValue = msStringConcatenate(styleValue, styleName);
        hashObj *hash = msInsertHashTable(StyleHashTable, styleName, styleValue);
        styleUrl = hash->data;
        msFree(styleValue);

            /* Insert new Style node into Document node*/
        xmlNodePtr styleNode = xmlNewChild(DocNode, NULL, BAD_CAST "Style", NULL);
        xmlNewProp(styleNode, BAD_CAST "id", BAD_CAST styleName);

        if (SymbologyFlag[Polygon])
        {
            xmlNodePtr polyStyleNode = xmlNewChild(styleNode, NULL, BAD_CAST "PolyStyle", NULL);
            xmlNewChild(polyStyleNode, NULL, BAD_CAST "color", BAD_CAST polygonHexColor);
        }

        if (SymbologyFlag[Line])
        {
            for (int i=0; i<numLineStyle; i++)
            {
                xmlNodePtr lineStyleNode = xmlNewChild(styleNode, NULL, BAD_CAST "LineStyle", NULL);
                sprintf(lineHexColor,"%02x%02x%02x%02x", LineStyle[i].color->alpha, LineStyle[i].color->blue,
                    LineStyle[i].color->green, LineStyle[i].color->red);
                xmlNewChild(lineStyleNode, NULL, BAD_CAST "color", BAD_CAST lineHexColor);

                char width[16];
                sprintf(width, "%.1f", LineStyle[i].width);
                xmlNewChild(lineStyleNode, NULL, BAD_CAST "width", BAD_CAST width);
            }
        }

        if (SymbologyFlag[Symbol])
        {
            xmlNodePtr iconStyleNode = xmlNewChild(styleNode, NULL, BAD_CAST "IconStyle", NULL);

            xmlNodePtr iconNode = xmlNewChild(iconStyleNode, NULL, BAD_CAST "Icon", NULL);
            xmlNewChild(iconNode, NULL, BAD_CAST "href", BAD_CAST SymbolUrl);

                /*char scale[16];
                sprintf(scale, "%.1f", style->scale);
                xmlNewChild(iconStyleNode, NULL, BAD_CAST "scale", BAD_CAST scale);*/
        }
        else
        {
            const char *value=msLookupHashTable(&currentLayer->metadata, "kml_default_symbol_href");
            if (value && strlen(value) > 0)
            {
                xmlNodePtr iconStyleNode = xmlNewChild(styleNode, NULL, BAD_CAST "IconStyle", NULL);

                xmlNodePtr iconNode = xmlNewChild(iconStyleNode, NULL, BAD_CAST "Icon", NULL);
                xmlNewChild(iconNode, NULL, BAD_CAST "href", BAD_CAST value);
            }
        }

        if (SymbologyFlag[Label])
        {
            xmlNodePtr labelStyleNode = xmlNewChild(styleNode, NULL, BAD_CAST "LabelStyle", NULL);
            xmlNewChild(labelStyleNode, NULL, BAD_CAST "color", BAD_CAST labelHexColor);

                /*char scale[16];
                sprintf(scale, "%.1f", style->scale);
                xmlNewChild(iconStyleNode, NULL, BAD_CAST "scale", BAD_CAST scale);*/
        }
    }

    if (styleName)
      msFree(styleName);

    return styleUrl;
}

void KmlRenderer::flushPlacemark()
{
    if (PlacemarkNode)
    {
        char *styleUrl = lookupPlacemarkStyle();
        xmlNewChild(PlacemarkNode, NULL, BAD_CAST "styleUrl", BAD_CAST styleUrl);
        
        if (DescriptionNode)
          xmlAddChild(PlacemarkNode, DescriptionNode);
        
        if (GeomNode)
          xmlAddChild(PlacemarkNode, GeomNode);
    }       
}


xmlNodePtr KmlRenderer::createDescriptionNode(shapeObj *shape)
{
      /*
	<description>
	<![CDATA[
	  special characters here
	]]> 
	<description>
      */


    /*description nodes for vector layers:
      - if kml_description is set, use it
      - if not, dump the attributes */

    if (pszLayerDescMetadata)
    {
        char *pszTmp=NULL;
        char *pszTmpDesc = NULL;
        size_t bufferSize = 0;
        pszTmpDesc = strdup(pszLayerDescMetadata);
        
        for (int i=0; i<currentLayer->numitems; i++)
        {
            bufferSize = strlen(currentLayer->items[i]) + 3;
            pszTmp = (char *)malloc(bufferSize);
            snprintf(pszTmp, bufferSize, "%%%s%%",currentLayer->items[i]);
            if (strcasestr(pszTmpDesc, pszTmp))
              pszTmpDesc = msCaseReplaceSubstring(pszTmpDesc,  pszTmp, shape->values[i]);
            msFree(pszTmp);
        }   
        xmlNodePtr descriptionNode = xmlNewNode(NULL, BAD_CAST "description");
        xmlNodeAddContent(descriptionNode, BAD_CAST pszTmpDesc);       
        msFree(pszTmpDesc);
        return descriptionNode;
    }
    else if (papszLayerIncludeItems && nIncludeItems > 0)
    {
        char lineBuf[512];
        xmlNodePtr descriptionNode = xmlNewNode(NULL, BAD_CAST "description");

        /*xmlNodeAddContent(descriptionNode, BAD_CAST "\n\t<![CDATA[\n");*/
        xmlNodeAddContent(descriptionNode, BAD_CAST "<table>");

        for (int i=0; i<currentLayer->numitems; i++)
        {
            int j=0;
            /*TODO optimize to calculate this only once per layer*/
            for (j=0; j<nIncludeItems;j++)
            {
                if (strcasecmp(currentLayer->items[i], papszLayerIncludeItems[j]) == 0)
                  break;
            }
            if (j<nIncludeItems)
            {
                if (shape->values[i] && strlen(shape->values[i]))
                  snprintf(lineBuf, sizeof(lineBuf), "<tr><td><b>%s</b></td><td>%s</td></tr>\n", currentLayer->items[i], shape->values[i]);
                else
                  snprintf(lineBuf, sizeof(lineBuf), "<tr><td><b>%s</b></td><td></td></tr>\n", currentLayer->items[i]);
                xmlNodeAddContent(descriptionNode, BAD_CAST lineBuf);
            }
           
        }

        xmlNodeAddContent(descriptionNode, BAD_CAST "</table>");
	/*xmlNodeAddContent(descriptionNode, BAD_CAST "\t]]>\n");*/

        return descriptionNode;
    }

    return NULL;
}

void KmlRenderer::addLineStyleToList(strokeStyleObj *style)
{
    /*actually this is not necessary. kml only uses the last LineStyle
      so we should not bother keeping them all*/
    int i =0;
    for (i=0; i<numLineStyle; i++)
    {
        if (style->width == LineStyle[i].width &&
            LineStyle[i].color->alpha == style->color->alpha &&
            LineStyle[i].color->red == style->color->red &&
            LineStyle[i].color->green == style->color->green &&
            LineStyle[i].color->blue == style->color->blue)
          break;
    }
    if (i == numLineStyle)
    {
        numLineStyle++;
        if (LineStyle == NULL)
          LineStyle = (strokeStyleObj *)malloc(sizeof(strokeStyleObj));
        else
          LineStyle = (strokeStyleObj *)realloc(LineStyle, sizeof(strokeStyleObj)*numLineStyle);

        memcpy(&LineStyle[numLineStyle-1], style, sizeof(strokeStyleObj));
    }

}

#endif

/**********************************************************************
 * $Id$
 *
 * Name:     mapogcsld.c
 * Project:  MapServer
 * Language: C
 * Purpose:  OGC SLD implementation
 * Author:   Y. Assefa, DM Solutions Group (assefa@dmsolutions.ca)
 *
 **********************************************************************
 * Copyright (c) 2003, Y. Assefa, DM Solutions Group Inc
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
 * Revision 1.4  2003/11/27 13:57:09  assefa
 * Add min/max scale.
 *
 * Revision 1.3  2003/11/25 03:21:44  assefa
 * Add test support.
 * Add filter support.
 *
 * Revision 1.2  2003/11/07 21:35:07  assefa
 * Add PointSymbolizer.
 * Add External Graphic symbol support.
 *
 * Revision 1.1  2003/11/06 23:09:25  assefa
 * OGC SLD support.
 *
 *
 **********************************************************************/

#include "mapogcsld.h"
#include "mapogcfilter.h"
#include "map.h"
#include "cpl_string.h"

#ifdef USE_OGR

#define SLD_LINE_SYMBOL_NAME "sld_line_symbol"
#define SLD_MARK_SYMBOL_SQUARE "sld_mark_symbol_square"
#define SLD_MARK_SYMBOL_SQUARE_FILLED "sld_mark_symbol_square_filled"
#define SLD_MARK_SYMBOL_CIRCLE "sld_mark_symbol_circle"
#define SLD_MARK_SYMBOL_CIRCLE_FILLED "sld_mark_symbol_circle_filled"
#define SLD_MARK_SYMBOL_TRIANGLE "sld_mark_symbol_triangle"
#define SLD_MARK_SYMBOL_TRIANGLE_FILLED "sld_mark_symbol_triangle_filled"
#define SLD_MARK_SYMBOL_STAR "sld_mark_symbol_star"
#define SLD_MARK_SYMBOL_STAR_FILLED "sld_mark_symbol_star_filled"
#define SLD_MARK_SYMBOL_CROSS "sld_mark_symbol_cross"
#define SLD_MARK_SYMBOL_CROSS_FILLED "sld_mark_symbol_cross_filled"
#define SLD_MARK_SYMBOL_X "sld_mark_symbol_x"
#define SLD_MARK_SYMBOL_X_FILLED "sld_mark_symbol_x_filled"



/************************************************************************/
/*                              msSLDApplySLD                           */
/*                                                                      */
/*      Parses the SLD into array of layers. Go through the map and     */
/*      compare the SLD layers and the map layers using the name. If    */
/*      they have the same name, copy the classes asscoaited with       */
/*      the SLD layers onto the map layers.                             */
/************************************************************************/
void msSLDApplySLD(mapObj *map, const char *wmtver, char *psSLDXML)
{
    int nLayers = 0;
    layerObj *pasLayers = NULL;
    int i, j, k, iClass;

    pasLayers = msSLDParseSLD(map, psSLDXML, &nLayers);


    if (pasLayers && nLayers > 0)
    {

        for (i=0; i<map->numlayers; i++)
        {
            for (j=0; j<nLayers; j++)
            {
                    
/* -------------------------------------------------------------------- */
/*      copy :  - class                                                 */
/*              - layer's labelitem                                     */
/* -------------------------------------------------------------------- */
                if (strcasecmp(map->layers[i].name, pasLayers[j].name) == 0)
                {
/* -------------------------------------------------------------------- */
/*      copy classes in reverse order : the Rule priority is the        */
/*      first rule is the most important (mapserver uses the painter    */
/*      model)                                                          */
/* -------------------------------------------------------------------- */
                    map->layers[i].numclasses = 0;
                    iClass = 0;
                    for (k=pasLayers[j].numclasses-1; k>=0; k--)
                    {
                        initClass(&map->layers[i].class[iClass]);
                        msCopyClass(&map->layers[i].class[iClass], 
                                    &pasLayers[j].class[k], NULL);
                        map->layers[i].class[iClass].layer = &map->layers[i];
                        map->layers[i].class[iClass].type = map->layers[i].type;
                        map->layers[i].numclasses++;
                        iClass++;
                    }

                    if (pasLayers[j].labelitem)
                    {
                        if (map->layers[i].labelitem)
                          free(map->layers[i].labelitem);

                        map->layers[i].labelitem = strdup(pasLayers[j].labelitem);
                    }

                    break;
                }
            }
        }
    }

    //test
    msSaveMap(map, "c:/msapps/ogc_cite/map/sld_map2.map");
}
                        
    
/************************************************************************/
/*                              msSLDParseSLD                           */
/*                                                                      */
/*      Parse the sld document into layers : for each named layer       */
/*      there is one mapserver layer created with approproate           */
/*      classes and styles.                                             */
/*      Returns an array of mapserver layers. The pnLayres if           */
/*      provided will indicate the size of the returned array.          */
/************************************************************************/
layerObj  *msSLDParseSLD(mapObj *map, char *psSLDXML, int *pnLayers)
{
    CPLXMLNode *psRoot = NULL;
    CPLXMLNode *psSLD, *psNamedLayer, *psChild, *psName;
    layerObj *pasLayers;
    int iLayer = 0;
    int nLayers = 0;

    if (map == NULL || psSLDXML == NULL || strlen(psSLDXML) <= 0 ||
        (strstr(psSLDXML, "StyledLayerDescriptor") == NULL))
    {
        msSetError(MS_WMSERR, "Invalid SLD document", "");
        return NULL;
    }

    psRoot = CPLParseXMLString(psSLDXML);
    if( psRoot == NULL)
    {
        msSetError(MS_WMSERR, "Invalid SLD document", "");
        return NULL;
    }

    //strip namespaces ogc and sld
    CPLStripXMLNamespace(psRoot, "ogc", 1); 
    CPLStripXMLNamespace(psRoot, "sld", 1); 
    

/* -------------------------------------------------------------------- */
/*      get the root element (Filter).                                  */
/* -------------------------------------------------------------------- */
    psChild = psRoot;
    psSLD = NULL;
     
    while( psChild != NULL )
    {
        if (psChild->eType == CXT_Element &&
            EQUAL(psChild->pszValue,"StyledLayerDescriptor"))
        {
            psSLD = psChild;
            break;
        }
        else
          psChild = psChild->psNext;
    }

    if (!psSLD)
    {
        msSetError(MS_WMSERR, "Invalid SLD document", "");
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Parse the named layers.                                         */
/* -------------------------------------------------------------------- */
    psNamedLayer = CPLGetXMLNode(psSLD, "NamedLayer");
    while (psNamedLayer &&  psNamedLayer->pszValue && 
           strcasecmp(psNamedLayer->pszValue, "NamedLayer") == 0)

    {
        psNamedLayer = psNamedLayer->psNext;
        nLayers++;
    }

    if (nLayers > 0)
      pasLayers = (layerObj *)malloc(sizeof(layerObj)*nLayers);

    psNamedLayer = CPLGetXMLNode(psSLD, "NamedLayer");
    if (psNamedLayer)
    {
        iLayer = 0;
        while (psNamedLayer &&  psNamedLayer->pszValue && 
               strcasecmp(psNamedLayer->pszValue, "NamedLayer") == 0)

        {
            psName = CPLGetXMLNode(psNamedLayer, "Name");
            initLayer(&pasLayers[iLayer], map);
            
            if (psName && psName->psChild &&  psName->psChild->pszValue)
              pasLayers[iLayer].name = strdup(psName->psChild->pszValue);
            
            msSLDParseNamedLayer(psNamedLayer, &pasLayers[iLayer]);
            
            psNamedLayer = psNamedLayer->psNext;
            iLayer++;
        }
    }

    if (pnLayers)
      *pnLayers = nLayers;

    if (psRoot)
      CPLDestroyXMLNode(psRoot);

    return pasLayers;
}


/************************************************************************/
/*                           msSLDParseNamedLayer                       */
/*                                                                      */
/*      Parse NamedLayer root.                                          */
/************************************************************************/
void msSLDParseNamedLayer(CPLXMLNode *psRoot, layerObj *psLayer)
{
    CPLXMLNode *psFeatureTypeStyle, *psRule, *psUserStyle;
    CPLXMLNode *psElseFilter = NULL, *psFilter=NULL;
    CPLXMLNode *psMinScale=NULL, *psMaxScale=NULL;
    CPLXMLNode *psTmpNode = NULL;
    FilterEncodingNode *psNode = NULL;
    char *szExpression = NULL;
    char *szClassItem = NULL;
    int i=0, nNewClasses=0, nClassBeforeFilter=0, nClassAfterFilter=0;
    int nClassAfterRule=0, nClassBeforeRule=0;
    char *pszTmpFilter = NULL;
    double dfMinScale=0, dfMaxScale=0;
    

    if (psRoot && psLayer)
    {
        psUserStyle = CPLGetXMLNode(psRoot, "UserStyle");
        if (psUserStyle)
        {
            psFeatureTypeStyle = CPLGetXMLNode(psUserStyle, "FeatureTypeStyle");
            if (psFeatureTypeStyle)
            {
                while (psFeatureTypeStyle && psFeatureTypeStyle->pszValue && 
                       strcasecmp(psFeatureTypeStyle->pszValue, 
                                  "FeatureTypeStyle") == 0)
                {
                    psRule = CPLGetXMLNode(psFeatureTypeStyle, "Rule");
/* -------------------------------------------------------------------- */
/*      First parse rules with the else filter. These rules will        */
/*      create the classes that are placed at the end of class          */
/*      list. (See how classes are applied to layers in function        */
/*      msSLDApplySLD).                                                 */
/* -------------------------------------------------------------------- */
                    while (psRule && psRule->pszValue && 
                           strcasecmp(psRule->pszValue, "Rule") == 0)
                    {
                        psElseFilter = CPLGetXMLNode(psRule, "ElseFilter");
                        if (psElseFilter)
                           msSLDParseRule(psRule, psLayer);
                         psRule = psRule->psNext;
                    }
/* -------------------------------------------------------------------- */
/*      Parse rules with no Else filter.                                */
/* -------------------------------------------------------------------- */
                    psRule = CPLGetXMLNode(psFeatureTypeStyle, "Rule");
                    while (psRule && psRule->pszValue && 
                           strcasecmp(psRule->pszValue, "Rule") == 0)
                    {
                        //used for scale setting
                        nClassBeforeRule = psLayer->numclasses;

                        psElseFilter = CPLGetXMLNode(psRule, "ElseFilter");
                        nClassBeforeFilter = psLayer->numclasses;
                        if (psElseFilter == NULL)
                          msSLDParseRule(psRule, psLayer);
                        nClassAfterFilter = psLayer->numclasses;

/* -------------------------------------------------------------------- */
/*      Parse the filter and apply it to the latest class created by    */
/*      the rule.                                                       */
/*      NOTE : Spatial Filter is not supported.                         */
/* -------------------------------------------------------------------- */
                        psFilter = CPLGetXMLNode(psRule, "Filter");
                        if (psFilter && psFilter->psChild && 
                            psFilter->psChild->pszValue)
                        {
                            //clone the tree and set the next node to null
                            //so we only have the Filter node
                            psTmpNode = CPLCloneXMLTree(psFilter);
                            psTmpNode->psNext = NULL;
                            pszTmpFilter = CPLSerializeXMLTree(psTmpNode);
                            CPLDestroyXMLNode(psTmpNode);

                            if (pszTmpFilter)
                            {
                            //nTmp = strlen(psFilter->psChild->pszValue)+17;
                            //pszTmpFilter = malloc(sizeof(char)*nTmp);
                            //sprintf(pszTmpFilter,"<Filter>%s</Filter>",
                            //        psFilter->psChild->pszValue);
                            //pszTmpFilter[nTmp-1]='\0';
                                psNode = FLTParseFilterEncoding(pszTmpFilter);
                            
                                CPLFree(pszTmpFilter);
                            }

                            if (psNode)
                            {
                              //TODO layer->classitem
                                szExpression = FLTGetMapserverExpression(psNode);
                                if (szExpression)
                                {
                                    szClassItem = 
                                      FLTGetMapserverExpressionClassItem(psNode);
                                    nNewClasses = nClassAfterFilter - nClassBeforeFilter;
                                    for (i=0; i<nNewClasses; i++)
                                    {
                                        loadExpressionString(&psLayer->
                                                             class[psLayer->numclasses-1-i].
                                                             expression, szExpression);
                                    }
                                    if (szClassItem)
                                      psLayer->classitem = strdup(szClassItem);
                                }
                            }
                        }
/* -------------------------------------------------------------------- */
/*      parse minscale and maxscale.                                    */
/* -------------------------------------------------------------------- */
                        psMinScale = CPLGetXMLNode(psRule, "MinScaleDenominator");
                        if (psMinScale && psMinScale->psChild && 
                            psMinScale->psChild->pszValue)
                          dfMinScale = atof(psMinScale->psChild->pszValue);

                        psMaxScale = CPLGetXMLNode(psRule, "MaxScaleDenominator");
                        if (psMaxScale && psMaxScale->psChild && 
                            psMaxScale->psChild->pszValue)
                          dfMaxScale = atof(psMaxScale->psChild->pszValue);

                        
/* -------------------------------------------------------------------- */
/*      set the scale to all the classes created by the rule.           */
/* -------------------------------------------------------------------- */
                        if (dfMinScale > 0 || dfMaxScale > 0)
                        {
                            nClassAfterRule = psLayer->numclasses;

                            nNewClasses = nClassAfterRule - nClassBeforeRule;
                            for (i=0; i<nNewClasses; i++)
                            {
                                if (dfMinScale > 0)
                                  psLayer->class[psLayer->numclasses-1-i].minscale = dfMinScale;
                                if (dfMaxScale)
                                  psLayer->class[psLayer->numclasses-1-i].maxscale = dfMaxScale;
                            }                           
                        }

                        psRule = psRule->psNext;

                    }
                    psFeatureTypeStyle = psFeatureTypeStyle->psNext;
                }
            }
        }
    }
}


/************************************************************************/
/*        void msSLDParseRule(CPLXMLNode *psRoot, layerObj *psLayer)    */
/*                                                                      */
/*      Parse a Rule node into classes for a spcific layer.             */
/************************************************************************/
void msSLDParseRule(CPLXMLNode *psRoot, layerObj *psLayer)
{
    CPLXMLNode *psLineSymbolizer = NULL;
    CPLXMLNode *psPolygonSymbolizer = NULL;
    CPLXMLNode *psPointSymbolizer = NULL;
    CPLXMLNode *psTextSymbolizer = NULL;
    CPLXMLNode *psMaxScale=NULL, *psMinScale=NULL;
    int i = 0;
    int bSymbolizer = 0;

    if (psRoot && psLayer)
    {
        //TODO : parse name of the rule
/* -------------------------------------------------------------------- */
/*      The SLD specs assumes here that a certain FeatureType can only have*/
/*      rules for only one type of symbolizer.                          */
/* -------------------------------------------------------------------- */
        //line symbolizer
        psLineSymbolizer = CPLGetXMLNode(psRoot, "LineSymbolizer");
        while (psLineSymbolizer && psLineSymbolizer->pszValue && 
               strcasecmp(psLineSymbolizer->pszValue, 
                          "LineSymbolizer") == 0)
        {
            bSymbolizer = 1;
            msSLDParseLineSymbolizer(psLineSymbolizer, psLayer);
            psLineSymbolizer = psLineSymbolizer->psNext;
        }

        //Polygon symbolizer
        psPolygonSymbolizer = CPLGetXMLNode(psRoot, "PolygonSymbolizer");
        while (psPolygonSymbolizer && psPolygonSymbolizer->pszValue && 
               strcasecmp(psPolygonSymbolizer->pszValue, 
                          "PolygonSymbolizer") == 0)
        {
            bSymbolizer = 1;
            msSLDParsePolygonSymbolizer(psPolygonSymbolizer, psLayer);
            psPolygonSymbolizer = psPolygonSymbolizer->psNext;
        }
        //Point Symbolizer
        psPointSymbolizer = CPLGetXMLNode(psRoot, "PointSymbolizer");
        while (psPointSymbolizer && psPointSymbolizer->pszValue && 
               strcasecmp(psPointSymbolizer->pszValue, 
                          "PointSymbolizer") == 0)
        {
            bSymbolizer = 1;
            msSLDParsePointSymbolizer(psPointSymbolizer, psLayer);
            psPointSymbolizer = psPointSymbolizer->psNext;
        }
        //Text symbolizer
/* ==================================================================== */
/*      For text symbolizer, here is how it is translated into          */
/*      mapserver classes :                                             */
/*        - If there are other symbolizers(line, polygon, symbol),      */
/*      the label object created will be created in the same class      */
/*      (the last class) as the  symbolizer. This allows o have for     */
/*      example of point layer with labels.                             */
/*        - If there are no other symbolizers, a new clas will be       */
/*      created ocontain the label object.                              */
/* ==================================================================== */
        psTextSymbolizer = CPLGetXMLNode(psRoot, "TextSymbolizer");
        while (psTextSymbolizer && psTextSymbolizer->pszValue && 
               strcasecmp(psTextSymbolizer->pszValue, 
                          "TextSymbolizer") == 0)
        {
            msSLDParseTextSymbolizer(psTextSymbolizer, psLayer, bSymbolizer);
            psTextSymbolizer = psTextSymbolizer->psNext;
        }

/* -------------------------------------------------------------------- */
/*      Parse the minscale and maxscale and applt it to all classes     */
/*      in the layer.                                                   */
/* -------------------------------------------------------------------- */
        psMinScale =  CPLGetXMLNode(psRoot, "MinScaleDenominator");
        if (psMinScale && psMinScale->psChild && 
            psMinScale->psChild->pszValue)
        {
            for (i=0; i<psLayer->numclasses; i++)
              psLayer->class[i].minscale = atof(psMinScale->psChild->pszValue);
        }

        psMaxScale =  CPLGetXMLNode(psRoot, "MaxScaleDenominator");
        if (psMaxScale && psMaxScale->psChild && 
            psMaxScale->psChild->pszValue)
        {
            for (i=0; i<psLayer->numclasses; i++)
              psLayer->class[i].maxscale = atof(psMaxScale->psChild->pszValue);
        }

    }
}


/************************************************************************/
/*        void msSLDParseLineSymbolizer(CPLXMLNode *psRoot, layerObj    */
/*      *psLayer)                                                       */
/*                                                                      */
/*      Parses the LineSymbolizer rule and creates a class in the       */
/*      layer.                                                          */
/*                                                                      */
/*      <xs:element name="LineSymbolizer">                              */
/*      <xs:complexType>                                                */
/*      <xs:sequence>                                                   */
/*      <xs:element ref="sld:Geometry" minOccurs="0"/>                  */
/*      <xs:element ref="sld:Stroke" minOccurs="0"/>                    */
/*      </xs:sequence>                                                  */
/*      </xs:complexType>                                               */
/*      </xs:element>                                                   */
/*                                                                      */
/*      <xs:element name="Stroke">                                      */
/*      <xs:complexType>                                                */
/*      <xs:sequence>                                                   */
/*      <xs:choice minOccurs="0">                                       */
/*      <xs:element ref="sld:GraphicFill"/>                             */
/*      <xs:element ref="sld:GraphicStroke"/>                           */
/*      </xs:choice>                                                    */
/*      <xs:element ref="sld:CssParameter" minOccurs="0"                */
/*      maxOccurs="unbounded"/>                                         */
/*      </xs:sequence>                                                  */
/*      </xs:complexType>                                               */
/*      </xs:element>                                                   */
/*                                                                      */
/*      Example of a rule :                                             */
/*      ...                                                             */
/*      <Rule>                                                          */
/*      <LineSymbolizer>                                                */
/*      <Geometry>                                                      */
/*      <ogc:PropertyName>center-line</ogc:PropertyName>                */
/*      </Geometry>                                                     */
/*      <Stroke>                                                        */
/*      <CssParameter name="stroke">#0000ff</CssParameter>              */
/*      <CssParameter name="stroke-width">5.0</CssParameter>            */
/*      <CssParameter name="stroke-dasharray">10.0 5 5 10</CssParameter>*/
/*      </Stroke>                                                       */
/*      </LineSymbolizer>                                               */
/*      </Rule>                                                         */
/*       ...                                                            */
/************************************************************************/
void msSLDParseLineSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer)
{
    int nClassId = 0;
    CPLXMLNode *psStroke;

    if (psRoot && psLayer)
    {
        psStroke =  CPLGetXMLNode(psRoot, "Stroke");
        if (psStroke)
        {
            initClass(&(psLayer->class[psLayer->numclasses]));
            nClassId = psLayer->numclasses;
            initStyle(&(psLayer->class[nClassId].styles[0]));
            psLayer->class[nClassId].numstyles = 1;
            psLayer->numclasses++;
            msSLDParseStroke(psStroke, &psLayer->class[nClassId].styles[0],
                             psLayer->map, 0); 
        }
    }
}



/************************************************************************/
/*           void msSLDParseStroke(CPLXMLNode *psStroke, styleObj       */
/*      *psStyle, int iColorParam)                                      */
/*                                                                      */
/*      Parse Stroke content into a style object.                       */
/*      The iColorParm is used to indicate which color object to use    */
/*      :                                                               */
/*        0 : for color                                                 */
/*        1 : outlinecolor                                              */
/*        2 : backgroundcolor                                           */
/************************************************************************/
void msSLDParseStroke(CPLXMLNode *psStroke, styleObj *psStyle,
                      mapObj *map, int iColorParam)
{
    CPLXMLNode *psCssParam = NULL, *psGraphicFill=NULL;
    char *psStrkName = NULL;
    char *psColor = NULL;
    int nLength = 0;
    char *pszDashValue = NULL;

    if (psStroke && psStyle)
    {
        //parse css parameters
        psCssParam =  CPLGetXMLNode(psStroke, "CssParameter");
            
        while (psCssParam && psCssParam->pszValue && 
               strcasecmp(psCssParam->pszValue, "CssParameter") == 0)
        {
            psStrkName = (char*)CPLGetXMLValue(psCssParam, "name", NULL);

            if (psStrkName)
            {
                if (strcasecmp(psStrkName, "stroke") == 0)
                {
                    if(psCssParam->psChild && psCssParam->psChild->psNext && 
                       psCssParam->psChild->psNext->pszValue)
                      psColor = psCssParam->psChild->psNext->pszValue;

                    if (psColor)
                    {
                        nLength = strlen(psColor);
                        //expecting hexadecimal ex : #aaaaff
                        if (nLength == 7 && psColor[0] == '#')
                        {
                            if (iColorParam == 0)
                            {
                                psStyle->color.red = hex2int(psColor+1);
                                psStyle->color.green = hex2int(psColor+3);
                                psStyle->color.blue= hex2int(psColor+5);
                            }
                            else if (iColorParam == 1)
                            {
                                psStyle->outlinecolor.red = hex2int(psColor+1);
                                psStyle->outlinecolor.green = hex2int(psColor+3);
                                psStyle->outlinecolor.blue= hex2int(psColor+5);
                            }
                            else if (iColorParam == 2)
                            {
                                psStyle->backgroundcolor.red = hex2int(psColor+1);
                                psStyle->backgroundcolor.green = hex2int(psColor+3);
                                psStyle->backgroundcolor.blue= hex2int(psColor+5);
                            }
                        }
                    }
                }
                else if (strcasecmp(psStrkName, "stroke-width") == 0)
                {
                    if(psCssParam->psChild &&  psCssParam->psChild->psNext && 
                       psCssParam->psChild->psNext->pszValue)
                    {
                        psStyle->size = 
                          atoi(psCssParam->psChild->psNext->pszValue);
                                
                        //use an ellipse symbol for the width
                        if (psStyle->symbol <=0)
                          psStyle->symbol = 
                            msSLDGetLineSymbol(map);
                    }
                }
                else if (strcasecmp(psStrkName, "stroke-dasharray") == 0)
                {
                    if(psCssParam->psChild && psCssParam->psChild->psNext &&
                       psCssParam->psChild->psNext->pszValue)
                    {
                        pszDashValue = strdup(psCssParam->psChild->psNext->pszValue);
                        //use an ellipse symbol with dash arrays
                        psStyle->symbol = 
                          msSLDGetDashLineSymbol(map, 
                                                 psCssParam->psChild->psNext->pszValue);
                    }
                }
            }
            psCssParam = psCssParam->psNext;
        }

        //parse graphic fill or stroke
        //graphic fill and graphic stroke pare parsed the same way : 
        //TODO : It seems inconsistent to me since the only diffrence
        //betewwn them seems to be fill (fill) or not fill (stroke). And
        //then again the fill parameter can be used inside both elements.
        psGraphicFill =  CPLGetXMLNode(psStroke, "GraphicFill");
        if (psGraphicFill)
          msSLDParseGraphicFillOrStroke(psGraphicFill, pszDashValue, psStyle, map);
        psGraphicFill =  CPLGetXMLNode(psStroke, "GraphicStroke");
        if (psGraphicFill)
          msSLDParseGraphicFillOrStroke(psGraphicFill, pszDashValue, psStyle, map);

        if (pszDashValue)
          free(pszDashValue);
    }
}



/************************************************************************/
/*           void msSLDParsePolygonSymbolizer(CPLXMLNode *psRoot,       */
/*      layerObj *psLayer)                                              */
/*                                                                      */
/*      <xs:element name="PolygonSymbolizer">                           */
/*      <xs:complexType>                                                */
/*      <xs:sequence>                                                   */
/*      <xs:element ref="sld:Geometry" minOccurs="0"/>                  */
/*      <xs:element ref="sld:Fill" minOccurs="0"/>                      */
/*      <xs:element ref="sld:Stroke" minOccurs="0"/>                    */
/*      </xs:sequence>                                                  */
/*      </xs:complexType>                                               */
/*      </xs:element>                                                   */
/*                                                                      */
/*                                                                      */
/*      <xs:element name="Fill">                                        */
/*      <xs:complexType>                                                */
/*      <xs:sequence>                                                   */
/*      <xs:element ref="sld:GraphicFill" minOccurs="0"/>               */
/*      <xs:element ref="sld:CssParameter" minOccurs="0"                */
/*      maxOccurs="unbounded"/>                                         */
/*      </xs:sequence>                                                  */
/*      </xs:complexType>                                               */
/*      </xs:element>                                                   */
/*                                                                      */
/*      Here, the CssParameter names are fill instead of stroke and     */
/*      fill-opacity instead of stroke-opacity. None of the other CssParameters*/
/*      in Stroke are available for filling and the default value for the fill color in this context is 50% gray (value #808080).*/
/*                                                                      */
/*                                                                      */
/*      <xs:element name="GraphicFill">                                 */
/*      <xs:complexType>                                                */
/*      <xs:sequence>                                                   */
/*      <xs:element ref="sld:Graphic"/>                                 */
/*      </xs:sequence>                                                  */
/*      </xs:complexType>                                               */
/*      </xs:element>                                                   */
/*                                                                      */
/*                                                                      */
/*      <xs:element name="Graphic">                                     */
/*      <xs:complexType>                                                */
/*      <xs:sequence>                                                   */
/*      <xs:choice minOccurs="0" maxOccurs="unbounded">                 */
/*      <xs:element ref="sld:ExternalGraphic"/>                         */
/*      <xs:element ref="sld:Mark"/>                                    */
/*      </xs:choice>                                                    */
/*      <xs:sequence>                                                   */
/*      <xs:element ref="sld:Opacity" minOccurs="0"/>                   */
/*      <xs:element ref="sld:Size" minOccurs="0"/>                      */
/*      <xs:element ref="sld:Rotation" minOccurs="0"/>                  */
/*      </xs:sequence>                                                  */
/*      </xs:sequence>                                                  */
/*      </xs:complexType>                                               */
/*      </xs:element>                                                   */
/*                                                                      */
/*      The default if neither an ExternalGraphic nor a Mark is specified is to use the default*/
/*      mark of a square with a 50%-gray fill and a black outline, with a size of 6 pixels.*/
/*                                                                      */
/*                                                                      */
/*      <xs:element name="Mark">                                        */
/*      <xs:complexType>                                                */
/*      <xs:sequence>                                                   */
/*      <xs:element ref="sld:WellKnownName" minOccurs="0"/>             */
/*      <xs:element ref="sld:Fill" minOccurs="0"/>                      */
/*      <xs:element ref="sld:Stroke" minOccurs="0"/>                    */
/*      </xs:sequence>                                                  */
/*      </xs:complexType>                                               */
/*      <xs:element name="WellKnownName" type="xs:string"/>             */
/*                                                                      */
/*      The WellKnownName element gives the well-known name of the shape of the mark.*/
/*      Allowed values include at least square, circle, triangle, star, cross,*/
/*      and x, though map servers may draw a different symbol instead if they don't have a*/
/*      shape for all of these. The default WellKnownName is square. Renderings of these*/
/*      marks may be made solid or hollow depending on Fill and Stroke elements.*/
/*                                                                      */
/************************************************************************/
void msSLDParsePolygonSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer)
{
    CPLXMLNode *psFill, *psStroke;
    int nClassId=0, nStyle=0;

    if (psRoot && psLayer)
    {
        psFill =  CPLGetXMLNode(psRoot, "Fill");
        if (psFill)
        {
            initClass(&(psLayer->class[psLayer->numclasses]));
            nClassId = psLayer->numclasses;
            initStyle(&(psLayer->class[nClassId].styles[0]));
            psLayer->class[nClassId].numstyles = 1;
            psLayer->numclasses++;
            msSLDParsePolygonFill(psFill, &psLayer->class[nClassId].styles[0],
                                  psLayer->map);
        }
        //stroke wich corresponds to the outilne in mapserver
        //is drawn after the fill
        psStroke =  CPLGetXMLNode(psRoot, "Stroke");
        if (psStroke)
        {
            if (psLayer->numclasses == 0)
            {
                initClass(&(psLayer->class[psLayer->numclasses]));
                nClassId = psLayer->numclasses;
                initStyle(&(psLayer->class[nClassId].styles[0]));
                psLayer->class[nClassId].numstyles = 1;
                psLayer->numclasses++;
                nStyle = 0;
            }
            else //there was a fill
            {
                nClassId =0;
                initStyle(&(psLayer->class[nClassId].styles[1]));
                psLayer->class[nClassId].numstyles++;
                nStyle = 1;
            }
            msSLDParseStroke(psStroke, &psLayer->class[nClassId].styles[nStyle],
                             psLayer->map, 1);
        }
    }
}
  

/************************************************************************/
/*    void msSLDParsePolygonFill(CPLXMLNode *psFill, styleObj *psStyle, */
/*                                 mapObj *map)                         */
/*                                                                      */
/*      Parse the Fill node for a polygon into a style.                 */
/************************************************************************/
void msSLDParsePolygonFill(CPLXMLNode *psFill, styleObj *psStyle,
                           mapObj *map)
{
    CPLXMLNode *psCssParam, *psGraphicFill;
    char *psColor=NULL, *psFillName=NULL;
    int nLength = 0;

    if (psFill && psStyle && map)
    {
        //sets the default fill color defined in the spec #808080
        psStyle->color.red = 128;
        psStyle->color.green = 128;
        psStyle->color.blue = 128;

        psCssParam =  CPLGetXMLNode(psFill, "CssParameter");
        while (psCssParam && psCssParam->pszValue && 
               strcasecmp(psCssParam->pszValue, "CssParameter") == 0)
        {
            psFillName = (char*)CPLGetXMLValue(psCssParam, "name", NULL);
            if (psFillName)
            {
                if (strcasecmp(psFillName, "fill") == 0)
                {
                    if(psCssParam->psChild && psCssParam->psChild->psNext && 
                       psCssParam->psChild->psNext->pszValue)
                      psColor = psCssParam->psChild->psNext->pszValue;

                    if (psColor)
                    {
                        nLength = strlen(psColor);
                        //expecting hexadecimal ex : #aaaaff
                        if (nLength == 7 && psColor[0] == '#')
                        {
                            psStyle->color.red = hex2int(psColor+1);
                            psStyle->color.green = hex2int(psColor+3);
                            psStyle->color.blue= hex2int(psColor+5);
                        }
                    }
                }
            }
            psCssParam = psCssParam->psNext;
        }
        
        //graphic fill and graphic stroke pare parsed the same way : 
        //TODO : It seems inconsistent to me since the only diffrence
        //betewwn them seems to be fill (fill) or not fill (stroke). And
        //then again the fill parameter can be used inside both elements.
        psGraphicFill =  CPLGetXMLNode(psFill, "GraphicFill");
        if (psGraphicFill)
          msSLDParseGraphicFillOrStroke(psGraphicFill, NULL, psStyle, map);
        psGraphicFill =  CPLGetXMLNode(psFill, "GraphicStroke");
        if (psGraphicFill)
          msSLDParseGraphicFillOrStroke(psGraphicFill, NULL, psStyle, map);

        
    }
}


/************************************************************************/
/*                      msSLDParseGraphicFillOrStroke                   */
/*                                                                      */
/*      Parse the GraphicFill Or GraphicStroke node : look for a        */
/*      Merker symbol and set the style for that symbol.                */
/************************************************************************/
void msSLDParseGraphicFillOrStroke(CPLXMLNode *psRoot, 
                                   char *pszDashValue,
                                   styleObj *psStyle, mapObj *map)
{
    CPLXMLNode  *psCssParam, *psGraphic, *psExternalGraphic, *psMark, *psSize;
    CPLXMLNode *psWellKnownName, *psStroke, *psFill;
    char *psColor=NULL, *psColorName = NULL, *psFillName=NULL;
    int nLength = 0;
    char *pszSymbolName = NULL;
    int bFilled = 0; 

    if (psRoot && psStyle && map)
    {
/* ==================================================================== */
/*      This a definition taken from the specification (11.3.2) :       */
/*      Graphics can either be referenced from an external URL in a common format (such as*/
/*      GIF or SVG) or may be derived from a Mark. Multiple external URLs and marks may be*/
/*      referenced with the semantic that they all provide the equivalent graphic in different*/
/*      formats.                                                        */
/*                                                                      */
/*      For this reason, we only need to support one Mark and one       */
/*      ExtrnalGraphic ????                                             */
/* ==================================================================== */
        psGraphic =  CPLGetXMLNode(psRoot, "Graphic");
        if (psGraphic)
        {
            //extract symbol size
            psSize = CPLGetXMLNode(psGraphic, "Size");
            if (psSize && psSize->psChild && psSize->psChild->pszValue)
              psStyle->size = atoi(psSize->psChild->pszValue);
            else
              psStyle->size = 6; //defualt value

            //extract symbol
            psMark =  CPLGetXMLNode(psGraphic, "Mark");
            if (psMark)
            {
                pszSymbolName = NULL;
                psWellKnownName =  CPLGetXMLNode(psMark, "WellKnownName");
                if (psWellKnownName && psWellKnownName->psChild &&
                    psWellKnownName->psChild->pszValue)
                  pszSymbolName = 
                    strdup(psWellKnownName->psChild->pszValue);
                    
                //default symbol is square
                if (!pszSymbolName || 
                    (strcasecmp(pszSymbolName, "square") != 0 &&
                     strcasecmp(pszSymbolName, "circle") != 0 &&
                     strcasecmp(pszSymbolName, "triangle") != 0 &&
                     strcasecmp(pszSymbolName, "star") != 0 &&
                     strcasecmp(pszSymbolName, "cross") != 0 &&
                     strcasecmp(pszSymbolName, "x") != 0))
                  pszSymbolName = strdup("square");
                    
                //set the default color
                psStyle->color.red = 128;
                psStyle->color.green = 128;
                psStyle->color.blue = 128;
                
                //check if the symbol should be filled or not
                psFill = CPLGetXMLNode(psMark, "Fill");
                psStroke = CPLGetXMLNode(psMark, "Stroke");
                    
                if (psFill || psStroke)
                {
                    if (psFill)
                      bFilled = 1;
                    else
                      bFilled = 0;

                    psCssParam =  CPLGetXMLNode(psFill, "CssParameter");
                    while (psCssParam && psCssParam->pszValue && 
                           strcasecmp(psCssParam->pszValue, "CssParameter") == 0)
                    {
                        psColorName = 
                          (char*)CPLGetXMLValue(psCssParam, "name", NULL);
                        if (psColorName && 
                            (strcasecmp(psColorName, "fill") == 0 ||
                             strcasecmp(psColorName, "stroke") == 0)) 
                        {
                            if(psCssParam->psChild && 
                               psCssParam->psChild->psNext && 
                               psCssParam->psChild->psNext->pszValue)
                              psColor = psCssParam->psChild->psNext->pszValue;

                            if (psColor)
                            {
                                nLength = strlen(psColor);
                                if (nLength == 7 && psColor[0] == '#')
                                  msSLDSetColorObject(psColor,  &psStyle->color);
                            }
                            break;
                        }
                        else
                          psCssParam = psCssParam->psNext;
                    }
                }
                //Get the corresponding symbol id 
                psStyle->symbol = msSLDGetMarkSymbol(map, pszSymbolName, 
                                                     bFilled, pszDashValue);
                    
            }
            else
            {
                psExternalGraphic =  CPLGetXMLNode(psGraphic, "ExternalGraphic");
                if (psExternalGraphic)
                  msSLDParseExternalGraphic(psExternalGraphic, psStyle, map);
            }
        }
    }
}
  
/************************************************************************/
/*                   int msSLDGetLineSymbol(mapObj *map)                */
/*                                                                      */
/*      Returns a symbol id for SLD_LINE_SYMBOL_NAME used for line      */
/*      with. If the symbol does not exist, cretaes it an inmap         */
/*      symbol.                                                         */
/************************************************************************/
int msSLDGetLineSymbol(mapObj *map)
{
    int nSymbolId = 0;
    symbolObj *psSymbol = NULL;

/* -------------------------------------------------------------------- */
/*      If the symbol exists, return it. We will use the same           */
/*      ellipse symbol for all the line width needs in the SLD.         */
/* -------------------------------------------------------------------- */
    nSymbolId = msGetSymbolIndex(&map->symbolset, SLD_LINE_SYMBOL_NAME);
    if (nSymbolId >= 0)
      return nSymbolId;

    if(map->symbolset.numsymbols == MS_MAXSYMBOLS) 
    { 
	msSetError(MS_SYMERR, "Too many symbols defined.", "msSLDGetLineSymbol()");
        return 0; //returs 0 for no symbol
    }

    psSymbol = &map->symbolset.symbol[map->symbolset.numsymbols];
    map->symbolset.numsymbols++;

 
/* -------------------------------------------------------------------- */
/*      Create an ellipse symbol to be used for lines :                 */
/*      NAME 'dashed'                                                   */
/*          TYPE ELLIPSE                                                */
/*        POINTS 1 1 END                                                */
/*        FILLED true                                                   */
/* -------------------------------------------------------------------- */
    initSymbol(psSymbol);
    psSymbol->inmapfile = MS_TRUE;

    psSymbol->name = strdup(SLD_LINE_SYMBOL_NAME);
    psSymbol->type = MS_SYMBOL_ELLIPSE;
    psSymbol->filled = MS_TRUE;
    psSymbol->points[psSymbol->numpoints].x = 1;
    psSymbol->points[psSymbol->numpoints].y = 1;
    psSymbol->sizex = 1;
    psSymbol->sizey = 1; 
    psSymbol->numpoints ++;

    return  map->symbolset.numsymbols-1;
}
  

/************************************************************************/
/*       int msSLDGetDashLineSymbol(mapObj *map, char *pszDashArray)    */
/*                                                                      */
/*      Create a dash line inmap symbol.                                */
/************************************************************************/
int msSLDGetDashLineSymbol(mapObj *map, char *pszDashArray)
{
    int nSymbolId = 0;
    symbolObj *psSymbol = NULL;
    char **aszValues = NULL;
    int nDash, i;


    if(map->symbolset.numsymbols == MS_MAXSYMBOLS) 
    { 
	msSetError(MS_SYMERR, "Too many symbols defined.", "msSLDGetDashLineSymbol()");
        return 0; //returs 0 for no symbol
    }

    psSymbol = &map->symbolset.symbol[map->symbolset.numsymbols];
    map->symbolset.numsymbols++;

 
/* -------------------------------------------------------------------- */
/*      Create an ellipse symbol to be used for lines :                 */
/*      NAME 'dashed'                                                   */
/*          TYPE ELLIPSE                                                */
/*        POINTS 1 1 END                                                */
/*        FILLED true                                                   */
/*        STYLE dashline value                                          */
/* -------------------------------------------------------------------- */
    initSymbol(psSymbol);
    psSymbol->inmapfile = MS_TRUE;

    psSymbol->name = strdup(SLD_LINE_SYMBOL_NAME);
    psSymbol->type = MS_SYMBOL_ELLIPSE;
    psSymbol->filled = MS_TRUE;
    psSymbol->points[psSymbol->numpoints].x = 1;
    psSymbol->points[psSymbol->numpoints].y = 1;
    psSymbol->sizex = 1;
    psSymbol->sizey = 1; 
    psSymbol->numpoints++;

    if (pszDashArray)
    {
        nDash = 0;
        aszValues = split(pszDashArray, ' ', &nDash);
        if (nDash > 0)
        {
            psSymbol->stylelength = nDash;
            for (i=0; i<nDash; i++)
              psSymbol->style[i] = atoi(aszValues[i]);

            msFreeCharArray(aszValues, nDash);
        }
    }
    return  map->symbolset.numsymbols-1;
}



/************************************************************************/
/*                            msSLDGetMarkSymbol                        */
/*                                                                      */
/*      Get a Mark symbol using the name. Mark symbols can be           */
/*      square, circle, triangle, star, cross, x.                       */
/*      If the symbol does not exsist add it to the symbol list.        */
/************************************************************************/
int msSLDGetMarkSymbol(mapObj *map, char *pszSymbolName, int bFilled,
                       char *pszDashValue)
{
    int nSymbolId = 0;
    char **aszValues = NULL;
    int nDash, i;
    symbolObj *psSymbol = NULL;

    if (!map || !pszSymbolName)
      return 0;

    if (strcasecmp(pszSymbolName, "square") == 0)
    {
        if (bFilled)
          nSymbolId = msGetSymbolIndex(&map->symbolset, 
                                       SLD_MARK_SYMBOL_SQUARE_FILLED);
        else
          nSymbolId = msGetSymbolIndex(&map->symbolset, 
                                       SLD_MARK_SYMBOL_SQUARE);
    }   
    else if (strcasecmp(pszSymbolName, "circle") == 0)
    {
        
        if (bFilled)
          nSymbolId = msGetSymbolIndex(&map->symbolset, 
                                       SLD_MARK_SYMBOL_CIRCLE_FILLED);
        else
          nSymbolId = msGetSymbolIndex(&map->symbolset, 
                                       SLD_MARK_SYMBOL_CIRCLE);
    }
    else if (strcasecmp(pszSymbolName, "triangle") == 0)
    {
        
        if (bFilled)
          nSymbolId = msGetSymbolIndex(&map->symbolset, 
                                       SLD_MARK_SYMBOL_TRIANGLE_FILLED);
        else
          nSymbolId = msGetSymbolIndex(&map->symbolset, 
                                       SLD_MARK_SYMBOL_TRIANGLE);
    }
    else if (strcasecmp(pszSymbolName, "star") == 0)
    {
        
        if (bFilled)
          nSymbolId = msGetSymbolIndex(&map->symbolset, 
                                       SLD_MARK_SYMBOL_STAR_FILLED);
        else
          nSymbolId = msGetSymbolIndex(&map->symbolset, 
                                       SLD_MARK_SYMBOL_STAR);
    }
    else if (strcasecmp(pszSymbolName, "cross") == 0)
    {
        
        if (bFilled)
          nSymbolId = msGetSymbolIndex(&map->symbolset, 
                                       SLD_MARK_SYMBOL_CROSS_FILLED);
        else
          nSymbolId = msGetSymbolIndex(&map->symbolset, 
                                       SLD_MARK_SYMBOL_CROSS);
    }
    else if (strcasecmp(pszSymbolName, "x") == 0)
    {
        
        if (bFilled)
          nSymbolId = msGetSymbolIndex(&map->symbolset, 
                                       SLD_MARK_SYMBOL_X_FILLED);
        else
          nSymbolId = msGetSymbolIndex(&map->symbolset, 
                                       SLD_MARK_SYMBOL_X);
    }


    if (nSymbolId <= 0)
    {
        if(map->symbolset.numsymbols == MS_MAXSYMBOLS) 
        {    
            msSetError(MS_SYMERR, "Too many symbols defined.", 
                       "msSLDGetMarkSymbol()");
            return 0; //returs 0 for no symbol
        }
        psSymbol = &map->symbolset.symbol[map->symbolset.numsymbols];
        nSymbolId = map->symbolset.numsymbols;
        map->symbolset.numsymbols++;
        initSymbol(psSymbol);
        psSymbol->inmapfile = MS_TRUE;
        psSymbol->sizex = 1;
        psSymbol->sizey = 1; 
        
        if (pszDashValue)
        {
            nDash = 0;
            aszValues = split(pszDashValue, ' ', &nDash);
            if (nDash > 0)
            {
                psSymbol->stylelength = nDash;
                for (i=0; i<nDash; i++)
                  psSymbol->style[i] = atoi(aszValues[i]);

                msFreeCharArray(aszValues, nDash);
            }
        }

        if (strcasecmp(pszSymbolName, "square") == 0)
        {
            if (bFilled)
              psSymbol->name = strdup(SLD_MARK_SYMBOL_SQUARE_FILLED);
            else
              psSymbol->name = strdup(SLD_MARK_SYMBOL_SQUARE);

            psSymbol->type = MS_SYMBOL_VECTOR;
            if (bFilled)
              psSymbol->filled = MS_TRUE;
            psSymbol->points[psSymbol->numpoints].x = 0;
            psSymbol->points[psSymbol->numpoints].y = 1;
            psSymbol->numpoints++;
            psSymbol->points[psSymbol->numpoints].x = 0;
            psSymbol->points[psSymbol->numpoints].y = 0;
            psSymbol->numpoints++;
            psSymbol->points[psSymbol->numpoints].x = 1;
            psSymbol->points[psSymbol->numpoints].y = 0;
            psSymbol->numpoints++;
            psSymbol->points[psSymbol->numpoints].x = 1;
            psSymbol->points[psSymbol->numpoints].y = 1;
            psSymbol->numpoints++;
            psSymbol->points[psSymbol->numpoints].x = 0;
            psSymbol->points[psSymbol->numpoints].y = 1;
            psSymbol->numpoints++;
        }
        else if (strcasecmp(pszSymbolName, "circle") == 0)
        {
            if (bFilled)
              psSymbol->name = strdup(SLD_MARK_SYMBOL_CIRCLE_FILLED);
            else
              psSymbol->name = strdup(SLD_MARK_SYMBOL_CIRCLE);
            
            psSymbol->type = MS_SYMBOL_ELLIPSE;
            if (bFilled)
              psSymbol->filled = MS_TRUE;

            psSymbol->points[psSymbol->numpoints].x = 1;
            psSymbol->points[psSymbol->numpoints].y = 1;
            psSymbol->sizex = 1;
            psSymbol->sizey = 1; 
            psSymbol->numpoints++;
        }
        else if (strcasecmp(pszSymbolName, "triangle") == 0)
        {
            if (bFilled)
              psSymbol->name = strdup(SLD_MARK_SYMBOL_TRIANGLE_FILLED);
            else
              psSymbol->name = strdup(SLD_MARK_SYMBOL_TRIANGLE);
            
            psSymbol->type = MS_SYMBOL_VECTOR;
            if (bFilled)
              psSymbol->filled = MS_TRUE;

            psSymbol->points[psSymbol->numpoints].x = 0;
            psSymbol->points[psSymbol->numpoints].y = 1;
            psSymbol->numpoints++;
            psSymbol->points[psSymbol->numpoints].x = 0.5;
            psSymbol->points[psSymbol->numpoints].y = 0;
            psSymbol->numpoints++;
            psSymbol->points[psSymbol->numpoints].x = 1;
            psSymbol->points[psSymbol->numpoints].y = 1;
            psSymbol->numpoints++;
            psSymbol->points[psSymbol->numpoints].x = 0;
            psSymbol->points[psSymbol->numpoints].y = 1;
            psSymbol->numpoints++;
        
        }
        else if (strcasecmp(pszSymbolName, "star") == 0)
        {
            if (bFilled)
              psSymbol->name = strdup(SLD_MARK_SYMBOL_STAR_FILLED);
            else
              psSymbol->name = strdup(SLD_MARK_SYMBOL_STAR);

            psSymbol->type = MS_SYMBOL_VECTOR;
            if (bFilled)
              psSymbol->filled = MS_TRUE;

            psSymbol->points[psSymbol->numpoints].x = 0;
            psSymbol->points[psSymbol->numpoints].y = 0.375;
            psSymbol->numpoints++;
            psSymbol->points[psSymbol->numpoints].x = 0.35;
            psSymbol->points[psSymbol->numpoints].y = 0.375;
            psSymbol->numpoints++;
            psSymbol->points[psSymbol->numpoints].x = 0.5;
            psSymbol->points[psSymbol->numpoints].y = 0;
            psSymbol->numpoints++;
            psSymbol->points[psSymbol->numpoints].x = 0.65;
            psSymbol->points[psSymbol->numpoints].y = 0.375;
            psSymbol->numpoints++;
            psSymbol->points[psSymbol->numpoints].x = 1;
            psSymbol->points[psSymbol->numpoints].y = 0.375;
            psSymbol->numpoints++;
            psSymbol->points[psSymbol->numpoints].x = 0.75;
            psSymbol->points[psSymbol->numpoints].y = 0.625;
            psSymbol->numpoints++;
            psSymbol->points[psSymbol->numpoints].x = 0.875;
            psSymbol->points[psSymbol->numpoints].y = 1;
            psSymbol->numpoints++;
            psSymbol->points[psSymbol->numpoints].x = 0.5;
            psSymbol->points[psSymbol->numpoints].y = 0.75;
            psSymbol->numpoints++;
            psSymbol->points[psSymbol->numpoints].x = 0.125;
            psSymbol->points[psSymbol->numpoints].y = 1;
            psSymbol->numpoints++;
            psSymbol->points[psSymbol->numpoints].x = 0.25;
            psSymbol->points[psSymbol->numpoints].y = 0.625;
            psSymbol->numpoints++;
        }
        //cross is like plus (+) since there is also X symbol ??
        else if (strcasecmp(pszSymbolName, "cross") == 0)
        {
            if (bFilled)
              psSymbol->name = strdup(SLD_MARK_SYMBOL_CROSS_FILLED);
            else
              psSymbol->name = strdup(SLD_MARK_SYMBOL_CROSS);
            
            psSymbol->type = MS_SYMBOL_VECTOR;
            if (bFilled)
              psSymbol->filled = MS_TRUE;

            psSymbol->points[psSymbol->numpoints].x = 0.5;
            psSymbol->points[psSymbol->numpoints].y = 0;
            psSymbol->numpoints++;
            psSymbol->points[psSymbol->numpoints].x = 0.5;
            psSymbol->points[psSymbol->numpoints].y = 1;
            psSymbol->numpoints++;
            psSymbol->points[psSymbol->numpoints].x = -1;
            psSymbol->points[psSymbol->numpoints].y = -1;
            psSymbol->numpoints++;
            psSymbol->points[psSymbol->numpoints].x = 0;
            psSymbol->points[psSymbol->numpoints].y = 0.5;
            psSymbol->numpoints++;
            psSymbol->points[psSymbol->numpoints].x = 1;
            psSymbol->points[psSymbol->numpoints].y = 0.5;
            psSymbol->numpoints++;
        }
        else if (strcasecmp(pszSymbolName, "x") == 0)
        {
            if (bFilled)
              psSymbol->name = strdup(SLD_MARK_SYMBOL_X_FILLED);
            else
              psSymbol->name = strdup(SLD_MARK_SYMBOL_X);
            
            psSymbol->type = MS_SYMBOL_VECTOR;
            if (bFilled)
              psSymbol->filled = MS_TRUE;
            psSymbol->points[psSymbol->numpoints].x = 0;
            psSymbol->points[psSymbol->numpoints].y = 0;
            psSymbol->numpoints++;
            psSymbol->points[psSymbol->numpoints].x = 1;
            psSymbol->points[psSymbol->numpoints].y = 1;
            psSymbol->numpoints++;
            psSymbol->points[psSymbol->numpoints].x = -1;
            psSymbol->points[psSymbol->numpoints].y = -1;
            psSymbol->numpoints++;
            psSymbol->points[psSymbol->numpoints].x = 0;
            psSymbol->points[psSymbol->numpoints].y = 1;
            psSymbol->numpoints++;
            psSymbol->points[psSymbol->numpoints].x = 1;
            psSymbol->points[psSymbol->numpoints].y = 0;
            psSymbol->numpoints++;
        }

    }

    return nSymbolId;
}

extern unsigned char PNGsig[8];
/************************************************************************/
/*                          msSLDGetGraphicSymbol                       */
/*                                                                      */
/*      Create a symbol entry for an inmap pixmap symbol. Returns       */
/*      the symbol id.                                                  */
/************************************************************************/
int msSLDGetGraphicSymbol(mapObj *map, char *pszFileName)
{
    FILE *fp;
    char bytes[8];
    gdImagePtr img = NULL;
    int nSymbolId = 0;
    symbolObj *psSymbol = NULL;


    if (map && pszFileName)
    {
        //check if a symbol of a 
        fp = fopen(pszFileName, "rb");
        if (fp)
        {
            fread(bytes,8,1,fp);
            rewind(fp);
            if (memcmp(bytes,"GIF8",4)==0)
            {
#ifdef USE_GD_GIF
                img = gdImageCreateFromGif(fp);
#endif
            }
            else if (memcmp(bytes,PNGsig,8)==0) 
            {
#ifdef USE_GD_PNG
                img = gdImageCreateFromPng(fp);
#endif        
            }
            fclose(fp);

            if (img)
            {
                psSymbol = &map->symbolset.symbol[map->symbolset.numsymbols];
                nSymbolId = map->symbolset.numsymbols;
                map->symbolset.numsymbols++;
                initSymbol(psSymbol);
                psSymbol->inmapfile = MS_TRUE;
                psSymbol->sizex = 1;
                psSymbol->sizey = 1; 
                psSymbol->type = MS_SYMBOL_PIXMAP;

                psSymbol->img = img;
            }
        }
    }
    return nSymbolId;
}
                 

/************************************************************************/
/*      msSLDParsePointSymbolizer                                       */
/*                                                                      */
/*      Parse point symbolizer.                                         */
/*                                                                      */
/*      <xs:element name="PointSymbolizer">                             */
/*      <xs:complexType>                                                */
/*      <xs:sequence>                                                   */
/*      <xs:element ref="sld:Geometry" minOccurs="0"/>                  */
/*      <xs:element ref="sld:Graphic" minOccurs="0"/>                   */
/*      </xs:sequence>                                                  */
/*      </xs:complexType>                                               */
/*      </xs:element>                                                   */
/************************************************************************/
void msSLDParsePointSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer)
{
    int nClassId = 0;
    if (psRoot && psLayer)
    {
        initClass(&(psLayer->class[psLayer->numclasses]));
        nClassId = psLayer->numclasses;
        initStyle(&(psLayer->class[nClassId].styles[0]));
        psLayer->class[nClassId].numstyles = 1;
        psLayer->numclasses++;
        msSLDParseGraphicFillOrStroke(psRoot, NULL,
                                      &psLayer->class[nClassId].styles[0],
                                      psLayer->map);
    }
}


/************************************************************************/
/*                        msSLDParseExternalGraphic                     */
/*                                                                      */
/*      Parse extrenal graphic node : download the symbol referneced    */
/*      by the URL and create a PIXMAP inmap symbol. Only GIF and       */
/*      PNG are supported.                                              */
/************************************************************************/
void msSLDParseExternalGraphic(CPLXMLNode *psExternalGraphic, 
                               styleObj *psStyle,  mapObj *map)
{
    char *pszFormat = NULL;
    CPLXMLNode *psURL=NULL, *psFormat=NULL;
    char *pszURL=NULL, *pszTmpSymbolName=NULL;
    int status;

    if (psExternalGraphic && psStyle && map)
    {
        psFormat = CPLGetXMLNode(psExternalGraphic, "Format");
        if (psFormat && psFormat->psChild && psFormat->psChild->pszValue)
          pszFormat = psFormat->psChild->pszValue;

        //supports GIF and PNG
        if (pszFormat && 
            (strcasecmp(pszFormat, "GIF") == 0 ||
             strcasecmp(pszFormat, "PNG") == 0))
        {
            psURL = CPLGetXMLNode(psExternalGraphic, "OnlineResource");
            if (psURL && psURL->psChild && psURL->psChild->pszValue)
            {
                pszURL = psURL->psChild->pszValue;

                if (strcasecmp(pszFormat, "GIF") == 0)
                  pszTmpSymbolName = msTmpFile(map->web.imagepath, "gif");
                else
                  pszTmpSymbolName = msTmpFile(map->web.imagepath, "png");

                if (msHTTPGetFile(pszURL, pszTmpSymbolName, &status,-1, 0, 0) ==  
                    MS_SUCCESS)
                {
                    psStyle->symbol = msSLDGetGraphicSymbol(map, pszTmpSymbolName);
                }
            }
        }
    }
}


/************************************************************************/
/*                         msSLDParseTextSymbolizer                     */
/*                                                                      */
/*      Parse text symbolizer.                                          */
/*                                                                      */
/*      <xs:element name="TextSymbolizer">                              */
/*      <xs:complexType>                                                */
/*      <xs:sequence>                                                   */
/*      <xs:element ref="sld:Geometry" minOccurs="0"/>                  */
/*      <xs:element ref="sld:Label" minOccurs="0"/>                     */
/*      <xs:element ref="sld:Font" minOccurs="0"/>                      */
/*      <xs:element ref="sld:LabelPlacement" minOccurs="0"/>            */
/*      <xs:element ref="sld:Halo" minOccurs="0"/>                      */
/*      <xs:element ref="sld:Fill" minOccurs="0"/>                      */
/*      </xs:sequence>                                                  */
/*      </xs:complexType>                                               */
/*      </xs:element>                                                   */
/*                                                                      */
/*      <xs:element name="Label" type="sld:ParameterValueType"/         */
/*                                                                      */
/*      <xs:element name="Font">                                        */
/*      <xs:complexType>                                                */
/*      <xs:sequence>                                                   */
/*      <xs:element ref="sld:CssParameter" minOccurs="0"                */
/*      maxOccurs="unbounded"/>                                         */
/*      </xs:sequence>                                                  */
/*      </xs:complexType>                                               */
/*      </xs:element>                                                   */
/*                                                                      */
/*      Four types of CssParameter are allowed, font-family, font-style,*/ 
/*      fontweight,and font-size.                                       */
/*                                                                      */
/*      <xs:element name="LabelPlacement">                              */
/*      <xs:complexType>                                                */
/*      <xs:choice>                                                     */
/*      <xs:element ref="sld:PointPlacement"/>                          */
/*      <xs:element ref="sld:LinePlacement"/>                           */
/*      </xs:choice>                                                    */
/*      </xs:complexType>                                               */
/*      </xs:element>                                                   */
/*                                                                      */
/*      <xs:element name="PointPlacement">                              */
/*      <xs:complexType>                                                */
/*      <xs:sequence>                                                   */
/*      <xs:element ref="sld:AnchorPoint" minOccurs="0"/>               */
/*      <xs:element ref="sld:Displacement" minOccurs="0"/>              */
/*      <xs:element ref="sld:Rotation" minOccurs="0"/>                  */
/*      </xs:sequence>                                                  */
/*      </xs:complexType>                                               */
/*      </xs:element>                                                   */
/*                                                                      */
/*      <xs:element name="AnchorPoint">                                 */
/*      <xs:complexType>                                                */
/*      <xs:sequence>                                                   */
/*      <xs:element ref="sld:AnchorPointX"/>                            */
/*      <xs:element ref="sld:AnchorPointY"/>                            */
/*      </xs:sequence>                                                  */
/*      </xs:complexType>                                               */
/*      </xs:element>                                                   */
/*      <xs:element name="AnchorPointX" type="sld:ParameterValueType"/> */
/*      <xs:element name="AnchorPointY"                                 */
/*      type="sld:ParameterValueType"/>                                 */
/*                                                                      */
/*      The coordinates are given as two floating-point numbers in      */
/*      the AnchorPointX and AnchorPointY elements each with values     */
/*      between 0.0 and 1.0 inclusive. The bounding box of the label    */
/*      to be rendered is considered to be in a coorindate space        */
/*      from 0.0 (lowerleft corner) to 1.0 (upper-right corner), and    */
/*      the anchor position is specified as a point in  this            */
/*      space. The default point is X=0, Y=0.5, which is at the         */
/*      middle height of the lefthand side of the label.                */
/*                                                                      */
/*      <xs:element name="Displacement">                                */
/*      <xs:complexType>                                                */
/*      <xs:sequence>                                                   */
/*      <xs:element ref="sld:DisplacementX"/>                           */
/*      <xs:element ref="sld:DisplacementY"/>                           */
/*      </xs:sequence>                                                  */
/*      </xs:complexType>                                               */
/*      </xs:element>                                                   */
/*      <xs:element name="DisplacementX" type="sld:ParameterValueType"/>*/
/*      <xs:element name="DisplacementY"                                */
/*      type="sld:ParameterValueType"/>                                 */
/*                                                                      */
/*      <xs:element name="LinePlacement">                               */
/*      <xs:complexType>                                                */
/*      <xs:sequence>                                                   */
/*      <xs:element ref="sld:PerpendicularOffset" minOccurs="0"/>       */
/*      </xs:sequence>                                                  */
/*      </xs:complexType>                                               */
/*      </xs:element>                                                   */
/************************************************************************/
void msSLDParseTextSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer,
                              int bOtherSymboliser)
{
    int nStyleId=0, nClassId=0;

    if (psRoot && psLayer)
    {
        if (!bOtherSymboliser)
        {
            initClass(&(psLayer->class[psLayer->numclasses]));
            nClassId = psLayer->numclasses;
            psLayer->numclasses++;
            initStyle(&(psLayer->class[nClassId].styles[0]));
            psLayer->class[nClassId].numstyles = 1;
            nStyleId = 0;
        }
        else
        {
            nClassId = psLayer->numclasses - 1;
            if (nClassId >= 0)//should always be true
              nStyleId = psLayer->class[nClassId].numstyles -1;
        }

        if (nStyleId >= 0 && nClassId >= 0) //should always be true
          msSLDParseTextParams(psRoot, psLayer,
                               &psLayer->class[nClassId]);
    }
}


/************************************************************************/
/*                           msSLDParseTextParams                       */
/*                                                                      */
/*      Parse text paramaters like font, placement and color.           */
/************************************************************************/
void msSLDParseTextParams(CPLXMLNode *psRoot, layerObj *psLayer, classObj *psClass)
{
    char szFontName[100];
    int  nFontSize = 10;
    int bFontSet = 0;

    CPLXMLNode *psLabel=NULL, *psPropertyName=NULL, *psFont=NULL;
    CPLXMLNode *psCssParam = NULL;
    char *pszName=NULL, *pszFontFamily=NULL, *pszFontStyle=NULL;
    char *pszFontWeight=NULL; 
    CPLXMLNode *psLabelPlacement=NULL, *psPointPlacement=NULL, *psLinePlacement=NULL;
    CPLXMLNode *psFill = NULL;
    int nLength = 0;
    char *pszColor = NULL;

    szFontName[0]='\0';

    if (psRoot && psClass && psLayer)
    {
        //label 
        //support only literal expression instead of propertyname
        psLabel = CPLGetXMLNode(psRoot, "Label");
        if (psLabel )
        {
            if (psLabel->psChild && psLabel->psChild->pszValue)
            //psPropertyName = CPLGetXMLNode(psLabel, "PropertyName");
              //if (psPropertyName && psPropertyName->psChild &&
              //psPropertyName->psChild->pszValue)
            {
                if (psLayer->labelitem)
                  free (psLayer->labelitem);
                psLayer->labelitem = strdup(psLabel->psChild->pszValue);
                  //strdup(psPropertyName->psChild->pszValue);

                //font
                psFont = CPLGetXMLNode(psRoot, "Font");
                if (psFont)
                {
                    psCssParam =  CPLGetXMLNode(psFont, "CssParameter");
                    while (psCssParam && psCssParam->pszValue && 
                           strcasecmp(psCssParam->pszValue, "CssParameter") == 0)
                    {
                        pszName = (char*)CPLGetXMLValue(psCssParam, "name", NULL);
                        if (pszName)
                        {
                            if (strcasecmp(pszName, "font-family") == 0)
                            {
                                if(psCssParam->psChild && psCssParam->psChild->psNext && 
                                  psCssParam->psChild->psNext->pszValue)
                                  pszFontFamily = psCssParam->psChild->psNext->pszValue;
                            }
                            //normal, italic, oblique
                            else if (strcasecmp(pszName, "font-style") == 0)
                            {
                                if(psCssParam->psChild && psCssParam->psChild->psNext && 
                                   psCssParam->psChild->psNext->pszValue)
                                  pszFontStyle = psCssParam->psChild->psNext->pszValue;
                            }
                            //normal or bold
                            else if (strcasecmp(pszName, "font-weight") == 0)
                            {
                                 if(psCssParam->psChild && psCssParam->psChild->psNext && 
                                   psCssParam->psChild->psNext->pszValue)
                                  pszFontWeight = psCssParam->psChild->psNext->pszValue;
                            }
                            //default is 10 pix
                            else if (strcasecmp(pszName, "font-size") == 0)
                            {

                                 if(psCssParam->psChild && psCssParam->psChild->psNext && 
                                    psCssParam->psChild->psNext->pszValue)
                                   nFontSize = atoi(psCssParam->psChild->psNext->pszValue); 
                                 if (nFontSize <=0)
                                   nFontSize = 10;
                            }
                        }
                        psCssParam = psCssParam->psNext;
                    }
                }
/* -------------------------------------------------------------------- */
/*      build the font name using the font font-family, font-style      */
/*      and font-weight. The name building uses a - between these       */
/*      parameters and the resulting name is compared to the list of    */
/*      available fonts. If the name exists, it will be used else we    */
/*      go to the bitmap fonts.                                         */
/* -------------------------------------------------------------------- */
                if (pszFontFamily)
                {
                    sprintf(szFontName, "%s", pszFontFamily);
                    if (pszFontWeight)
                    {
                        strcat(szFontName, "-");
                        strcat(szFontName, pszFontWeight);
                    }
                    if (pszFontStyle)
                    {
                        strcat(szFontName, "-");
                        strcat(szFontName, pszFontStyle);
                    }
                        
                    if ((msLookupHashTable(psLayer->map->fontset.fonts, szFontName) !=NULL))
                    {
                        bFontSet = 1;
                        psClass->label.font = strdup(szFontName);
                        psClass->label.type = MS_TRUETYPE;
                        psClass->label.size = nFontSize;
                    }
                }       
                if (!bFontSet)
                {
                    psClass->label.type = MS_BITMAP;
                    psClass->label.size = MS_MEDIUM;
                }
/* -------------------------------------------------------------------- */
/*      parse the label placement.                                      */
/* -------------------------------------------------------------------- */
                psLabelPlacement = CPLGetXMLNode(psRoot, "LabelPlacement");
                if (psLabelPlacement)
                {
                    psPointPlacement = CPLGetXMLNode(psLabelPlacement, 
                                                     "PointPlacement");
                    psLinePlacement = CPLGetXMLNode(psLabelPlacement, 
                                                     "LinePlacement");
                    if (psPointPlacement)
                      ParseTextPointPlacement(psPointPlacement, psClass);
                    if (psLinePlacement)
                      ParseTextLinePlacement(psPointPlacement, psClass);
                }

                
/* -------------------------------------------------------------------- */
/*      Parse the color                                                 */
/* -------------------------------------------------------------------- */
                psFill = CPLGetXMLNode(psRoot, "Fill");
                if (psFill)
                {
                    psCssParam =  CPLGetXMLNode(psFill, "CssParameter");
                    while (psCssParam && psCssParam->pszValue && 
                           strcasecmp(psCssParam->pszValue, "CssParameter") == 0)
                    {
                        pszName = (char*)CPLGetXMLValue(psCssParam, "name", NULL);
                        if (pszName)
                        {
                            if (strcasecmp(pszName, "fill") == 0)
                            {
                                if(psCssParam->psChild && psCssParam->psChild->psNext && 
                                   psCssParam->psChild->psNext->pszValue)
                                  pszColor = psCssParam->psChild->psNext->pszValue;

                                if (pszColor)
                                {
                                    nLength = strlen(pszColor);
                                    //expecting hexadecimal ex : #aaaaff
                                    if (nLength == 7 && pszColor[0] == '#')
                                    {
                                        psClass->label.color.red = hex2int(pszColor+1);
                                        psClass->label.color.green = hex2int(pszColor+3);
                                        psClass->label.color.blue = hex2int(pszColor+5);
                                    }
                                }
                            }
                        }
                        psCssParam = psCssParam->psNext;
                    }
                }
            
            }//labelitem
        }
    }
}

/************************************************************************/
/*                         ParseTextPointPlacement                      */
/*                                                                      */
/*      point plavament node ifor the text symbolizer.                  */
/************************************************************************/
void ParseTextPointPlacement(CPLXMLNode *psRoot, classObj *psClass)
{
    CPLXMLNode *psAnchor, *psAnchorX, *psAnchorY;
    double dfAnchorX=0, dfAnchorY=0;
    CPLXMLNode *psDisplacement, *psDisplacementX, *psDisplacementY;
    CPLXMLNode *psRotation;

    if (psRoot && psClass)
    {
        //init the label with the default position
        psClass->label.position = MS_CL;

/* -------------------------------------------------------------------- */
/*      parse anchor point. see function msSLDParseTextSymbolizer       */
/*      for documentation.                                              */
/* -------------------------------------------------------------------- */
        psAnchor = CPLGetXMLNode(psRoot, "AnchorPoint");
        if (psAnchor)
        {
            psAnchorX = CPLGetXMLNode(psAnchor, "AnchorPointX");
            psAnchorY = CPLGetXMLNode(psAnchor, "AnchorPointY");
            //psCssParam->psChild->psNext->pszValue)
            if (psAnchorX &&
                psAnchorX->psChild && 
                psAnchorX->psChild->pszValue &&
                psAnchorY && 
                psAnchorY->psChild && 
                psAnchorY->psChild->pszValue)
            {
                dfAnchorX = atof(psAnchorX->psChild->pszValue);
                dfAnchorY = atof(psAnchorY->psChild->pszValue);

                if ((dfAnchorX == 0 || dfAnchorX == 0.5 || dfAnchorX == 1) &&
                    (dfAnchorY == 0 || dfAnchorY == 0.5 || dfAnchorY == 1))
                {
                    if (dfAnchorX == 0 && dfAnchorY == 0)
                      psClass->label.position = MS_LL;
                    if (dfAnchorX == 0 && dfAnchorY == 0.5)
                      psClass->label.position = MS_CL;
                    if (dfAnchorX == 0 && dfAnchorY == 1)
                      psClass->label.position = MS_UL;

                    if (dfAnchorX == 0.5 && dfAnchorY == 0)
                      psClass->label.position = MS_LC;
                    if (dfAnchorX == 0.5 && dfAnchorY == 0.5)
                      psClass->label.position = MS_CC;
                    if (dfAnchorX == 0.5 && dfAnchorY == 1)
                      psClass->label.position = MS_UC;

                    if (dfAnchorX == 1 && dfAnchorY == 0)
                      psClass->label.position = MS_LR;
                    if (dfAnchorX == 1 && dfAnchorY == 0.5)
                      psClass->label.position = MS_CR;
                    if (dfAnchorX == 1 && dfAnchorY == 1)
                      psClass->label.position = MS_UR;
                }
            }
        }

/* -------------------------------------------------------------------- */
/*      Parse displacement                                              */
/* -------------------------------------------------------------------- */
        psDisplacement = CPLGetXMLNode(psRoot, "Displacement");
        if (psDisplacement)
        {
            psDisplacementX = CPLGetXMLNode(psDisplacement, "DisplacementX");
            psDisplacementY = CPLGetXMLNode(psDisplacement, "DisplacementY");
            //psCssParam->psChild->psNext->pszValue)
            if (psDisplacementX &&
                psDisplacementX->psChild && 
                psDisplacementX->psChild->pszValue &&
                psDisplacementY && 
                psDisplacementY->psChild && 
                psDisplacementY->psChild->pszValue)
            {
                psClass->label.offsetx = atoi(psDisplacementX->psChild->pszValue);
                psClass->label.offsety = atoi(psDisplacementY->psChild->pszValue);
            }
        }

/* -------------------------------------------------------------------- */
/*      parse rotation.                                                 */
/* -------------------------------------------------------------------- */
        psRotation = CPLGetXMLNode(psRoot, "Rotation");
        if (psRotation && psRotation->psChild && psRotation->psChild->pszValue)
          psClass->label.angle = atof(psRotation->psChild->pszValue);
    }
           
}

/************************************************************************/
/*                          ParseTextLinePlacement                      */
/*                                                                      */
/*      Lineplacement node fro the text symbolizer.                     */
/************************************************************************/
void ParseTextLinePlacement(CPLXMLNode *psRoot, classObj *psClass)
{
    CPLXMLNode *psOffset = NULL;
    if (psRoot && psClass)
    {
        psOffset = CPLGetXMLNode(psRoot, "PerpendicularOffset");
        if (psOffset && psOffset->psChild && psOffset->psChild->pszValue)
        {
            psClass->label.offsetx = atoi(psOffset->psChild->pszValue);
            psClass->label.offsety = atoi(psOffset->psChild->pszValue);
        }
    }
            
}


/************************************************************************/
/*           void msSLDSetColorObject(char *psHexColor, colorObj        */
/*      *psColor)                                                       */
/*                                                                      */
/*      Utility function to exctract rgb values from an hexadecimal     */
/*      color string (format is : #aaff08) and set it in the color      */
/*      object.                                                         */
/************************************************************************/
void msSLDSetColorObject(char *psHexColor, colorObj *psColor)
{
    int nLength = 0;
    if (psHexColor && psColor && strlen(psHexColor)== 7 && 
        psHexColor[0] == '#')
    {
        
        psColor->red = hex2int(psHexColor+1);
        psColor->green = hex2int(psHexColor+3);
        psColor->blue= hex2int(psHexColor+5);
    }
}     
#endif

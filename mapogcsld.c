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
 * Revision 1.1  2003/11/06 23:09:25  assefa
 * OGC SLD support.
 *
 *
 **********************************************************************/

#include "mapogcsld.h"
#include "map.h"

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
                if (strcasecmp(map->layers[i].name, pasLayers[j].name) == 0)
                {
                    map->layers[i].numclasses = 0;
                    //copy in reverse order : the Rule priority is 
                    //the first rule is the most important (mapserver
                    //uses the painter model)
                    iClass = 0;
                    for (k=pasLayers[j].numclasses-1; k>=0; k--)
                    {
                        initClass(&map->layers[i].class[iClass]);
                        msCopyClass(&map->layers[i].class[iClass], 
                                    &pasLayers[j].class[k], NULL);
                        map->layers[i].numclasses++;
                        iClass++;
                    }
                    break;
                }
            }
        }
    }
}
                        
    
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

    //strip namespaces
    CPLStripXMLNamespace(psRoot, NULL, 1); 

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
    return pasLayers;
}

void msSLDParseNamedLayer(CPLXMLNode *psRoot, layerObj *psLayer)
{
    CPLXMLNode *psFeatureTypeStyle, *psRule, *psUserStyle;
 
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
                    while (psRule && psRule->pszValue && 
                           strcasecmp(psRule->pszValue, "Rule") == 0)
                    {
                        msSLDParseRule(psRule, psLayer);
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
            msSLDParseLineSymbolizer(psLineSymbolizer, psLayer);
            psLineSymbolizer = psLineSymbolizer->psNext;
        }

        //Polygon symbolizer
        psPolygonSymbolizer = CPLGetXMLNode(psRoot, "PolygonSymbolizer");
        while (psPolygonSymbolizer && psPolygonSymbolizer->pszValue && 
               strcasecmp(psPolygonSymbolizer->pszValue, 
                          "PolygonSymbolizer") == 0)
        {
            msSLDParsePolygonSymbolizer(psPolygonSymbolizer, psLayer);
            psPolygonSymbolizer = psPolygonSymbolizer->psNext;
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
void msSLDParseGraphicFillOrStroke(CPLXMLNode *psGraphicFill, 
                                   char *pszDashValue,
                                   styleObj *psStyle, mapObj *map)
{
    CPLXMLNode  *psCssParam, *psGraphic, *psMark, *psSize;
    CPLXMLNode *psWellKnownName, *psStroke, *psFill;
    char *psColor=NULL, *psColorName = NULL, *psFillName=NULL;
    int nLength = 0;
    char *pszSymbolName = NULL;
    int bFilled = 0; 

    if (psGraphicFill && psStyle && map)
    {
        //only Mark symbols are supported.
        //TODO : External symbols
        psGraphic =  CPLGetXMLNode(psGraphicFill, "Graphic");
        if (psGraphic)
        {
            //extract symbol size
            psSize = CPLGetXMLNode(psGraphic, "Size");
            if (psSize && psSize->psChild && psSize->psChild->pszValue)
              psStyle->size = atoi(psSize->psChild->pszValue);

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

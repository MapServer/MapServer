/**********************************************************************
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
 * Revision 1.71  2006/09/26 12:18:09  assefa
 * [SLD] quantity values for raster sld can be float values instead of just
 * being integer
 *
 * Revision 1.70  2006/08/31 17:24:05  assefa
 * use Title of Rule if Name not present (bug 1889).
 *
 * Revision 1.69  2006/08/28 21:43:30  assefa
 * Add ifdefs around functions using ows functions.
 *
 * Revision 1.68  2006/08/28 20:16:43  assefa
 * PointSymbolizer does not use outline color correctly (bug 1887).
 *
 * Revision 1.67  2006/08/26 14:53:38  hobu
 * ensure that szCompare and szCompare2 are initialized before being used
 *
 * Revision 1.66  2006/08/25 14:03:17  assefa
 * Generate ogc filters now outputs the ocg name space (bug 1863).
 *
 * Revision 1.65  2006/08/23 18:06:46  assefa
 * Correct partly the problem of translating regex to ogc:Literal (bug 1644).
 *
 * Revision 1.64  2006/08/23 14:16:14  assefa
 * Initialize variables. Remove unused function.
 *
 * Revision 1.63  2006/08/22 18:20:50  assefa
 * Support <propertyname> tag in SLD label (Bug 1857)
 *
 * Revision 1.62  2006/08/22 17:33:32  assefa
 * Use the label element in the ColorMapEntry for the raster symbolizer
 * (Bug 1844).
 *
 * Revision 1.61  2006/05/26 02:21:13  assefa
 * Support symbols beside the WellKnownName ones for Mark symbols (Bug 1798).
 *
 * Revision 1.60  2006/02/24 02:14:04  assefa
 * Set the default color on the style when using default settings
 * in PointSymbolizer. (bug 1681)
 *
 * Revision 1.59  2006/02/01 19:35:40  assefa
 * When generating an ogc filter for class regex expressions, use
 * the backslah as the default escape character (Bug 1637)
 *
 * Revision 1.58  2005/12/06 15:06:24  assefa
 * Error parsing font parameters with the keyword "normal"
 *
 * Revision 1.57  2005/11/25 19:34:14  assefa
 * If a RULE name is not given, set the class name to "Unknown" (Bug 1451)
 *
 * Revision 1.56  2005/10/28 16:36:01  assefa
 * Syntax error when auto generating external symbols (Bug 1508),
 *
 * Revision 1.55  2005/07/26 16:23:20  assefa
 * SLD external graphic symbol format tests now for mime type
 * like image/gif instead of just GIF (Bug 1430).
 *
 * Revision 1.54  2005/04/21 23:46:07  assefa
 * Apply sld named layer on all layers of the same group : Bug 1329.
 *
 * Revision 1.53  2005/03/29 22:53:14  assefa
 * Initial support to improve WFS filter performance for DB layers (Bug 1292).
 *
 * Revision 1.52  2005/02/28 15:24:49  assefa
 * SLD generation bug 1150 : replacing <AND> tag to <ogc:And>
 *
 * Revision 1.51  2005/02/18 03:06:46  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
 *
 * Revision 1.50  2005/01/03 15:43:38  assefa
 * Correct bug 1151 : generates twice a </Mark> tag. This was happening the
 * style did not have a size set.
 *
 * Revision 1.49  2004/12/09 22:17:27  assefa
 * Delete temporary sld file created on disk (Bug 1123)
 *
 * Revision 1.48  2004/11/25 21:25:20  assefa
 * Make sure that spatial filters are not applied on raster layers (Bug 1987).
 *
 * Revision 1.47  2004/11/17 17:24:52  dan
 * Fixed warnings introduced by last change
 *
 * Revision 1.46  2004/11/17 15:59:57  assefa
 * replace lookup for wms_name to wms/ows_name (Bug  568).
 *
 * Revision 1.45  2004/11/01 17:28:42  assefa
 * HTML encode names and filter when generating SLD (Bug 892).
 *
 * Revision 1.44  2004/10/29 22:18:54  assefa
 * Use ows_schama_location metadata. The default value if metadata is not found
 * is http://schemas.opengeospatial.net
 *
 * Revision 1.43  2004/10/29 19:56:19  assefa
 * Use a class name as the RULE name when generating an SLD.
 *
 * Revision 1.42  2004/10/25 20:57:18  assefa
 * Comments between NamedLayers was causing the rest of NamedLayers to
 * be ignored (Bug 741)
 *
 * Revision 1.41  2004/10/21 04:30:56  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 * Revision 1.40  2004/10/09 18:22:41  sean
 * towards resolving bug 339, have implemented a mutex acquiring wrapper for
 * the loadExpressionString function.  the new msLoadExpressionString should be
 * used everywhere outside of the mapfile loading phase, and the previous
 * loadExpressionString function should be used within the mapfile loading
 * phase.
 *
 * Revision 1.39  2004/09/14 15:15:01  assefa
 * Correct bug related to sld generation for polygon layers (Bug 866)
 *
 * Revision 1.38  2004/08/17 17:53:01  assefa
 * Correct bug when generating sld filters based on expressions.
 *
 * Revision 1.37  2004/07/29 21:50:19  assefa
 * Use wfs_filter metedata when generating an SLD (Bug 782)
 *
 * Revision 1.36  2004/07/28 22:16:17  assefa
 * Add support for spatial filters inside an SLD. (Bug 782).
 *
 * Revision 1.35  2004/07/13 20:39:36  dan
 * Made msTmpFile() more robust using msBuildPath() to return absolute paths (bug 771)
 *
 * Revision 1.34  2004/06/23 21:33:27  assefa
 * Correct bug related to onlineresource (Bug 739).
 *
 * Revision 1.33  2004/06/22 20:55:20  sean
 * Towards resolving issue 737 changed hashTableObj to a structure which contains a hashObj **items.  Changed all hash table access functions to operate on the target table by reference.  msFreeHashTable should not be used on the hashTableObj type members of mapserver structures, use msFreeHashItems instead.
 *
 * Revision 1.32  2004/06/18 21:08:55  assefa
 * Initialize symbol name in msSLDGetGraphicSymbol.
 *
 * Revision 1.31  2004/06/18 16:13:33  assefa
 * Set Scale/Title/Name for Else filter (Bug 735)
 *
 * Revision 1.30  2004/06/17 20:26:33  assefa
 * Comment lines between Rules (or betwwen Symbolizers) was causing the
 * rest of the Rules to be ignored. Bug 731.
 *
 * Revision 1.29  2004/06/14 17:26:13  assefa
 * Add opacity support for raster symbolizer.
 *
 * Revision 1.28  2004/04/16 20:19:38  dan
 * Added try_addimage_if_notfound to msGetSymbolIndex() (bug 612)
 *
 * Revision 1.27  2004/04/16 19:12:31  assefa
 * Correct bug on windows when opening xml file (open it in binary mode).
 *
 * Revision 1.26  2004/03/15 07:36:15  frank
 * removed unused variables
 *
 * Revision 1.25  2004/03/11 20:42:42  assefa
 * Correct validation issues with the generated sld.
 *
 * Revision 1.24  2004/02/27 16:27:15  assefa
 * Add support for min/max scale in generatesld.
 *
 * Revision 1.23  2004/02/20 01:01:10  assefa
 * Check the wms_name when applying an sld.
 * Set the layer types when applying an sld.
 *
 * Revision 1.22  2004/02/12 16:01:06  assefa
 * Test missing in Generate SLD for annotation layers.
 *
 * Revision 1.21  2004/02/11 20:47:18  assefa
 * Use first the wms_name metadata as the name of the NamedLayer.
 * If not available, use the layer's name.
 *
 * Revision 1.20  2004/02/09 22:02:33  assefa
 * Forgot to remove debug statements.
 *
 * Revision 1.19  2004/02/09 21:42:02  assefa
 * Add RasterSymbolizer support.
 *
 * Revision 1.18  2004/02/06 02:23:01  assefa
 * Make sure that point symbolizers always initialize the color
 * parameter of the style.
 *
 * Revision 1.17  2004/02/03 23:48:22  assefa
 * Correct a bug in msSLDApplySLD.
 *
 * Revision 1.16  2004/01/07 19:02:53  assefa
 * Correct return value on applysld functions.
 * Add ifdef in functions using libcurl related functions (httpxxx).
 *
 * Revision 1.15  2004/01/05 21:17:53  assefa
 * ApplySLD and ApplySLDURL on a layer can now take a NamedLayer name as argument.
 *
 * Revision 1.14  2003/12/18 18:58:55  assefa
 * Use the symol name instead of the id for newly created symbols.
 *
 * Revision 1.13  2003/12/17 04:16:15  frank
 * added ifdef USE_OGR around include of cpl_string.h
 *
 * Revision 1.12  2003/12/11 05:12:27  assefa
 * Remove unused variables.
 *
 * Revision 1.11  2003/12/10 20:56:17  assefa
 * Generate default symbol (square) when having an "invalid" symbol.
 *
 * Revision 1.10  2003/12/10 17:36:03  assefa
 * Add partly support for Expressions.
 * Correct bug with symbol outline.
 *
 * Revision 1.9  2003/12/05 04:02:33  assefa
 * Add generation of SLD for points and text.
 *
 * Revision 1.8  2003/12/03 18:52:21  assefa
 * Add partly support for SLD generation.
 *
 * Revision 1.7  2003/12/01 16:10:13  assefa
 * Add #ifdef USE_OGR for sld functions available to mapserver.
 *
 * Revision 1.6  2003/11/30 16:30:04  assefa
 * Support mulitple symbolisers in a Rule.
 *
 * Revision 1.5  2003/11/27 15:04:30  assefa
 * Remove unused varaibeles.
 *
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

#ifdef USE_OGR
#include "cpl_string.h"
#endif

MS_CVSID("$Id$")

#define SLD_LINE_SYMBOL_NAME "sld_line_symbol"
#define SLD_LINE_SYMBOL_DASH_NAME "sld_line_symbol_dash"
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
/*                             msSLDApplySLDURL                         */
/*                                                                      */
/*      Use the SLD document given through a URL and apply the SLD      */
/*      on the map. Layer name and Named Layer's name parameter are     */
/*      used to do the match.                                           */
/************************************************************************/
int msSLDApplySLDURL(mapObj *map, char *szURL, int iLayer,
                     char *pszStyleLayerName)
{
#ifdef USE_OGR

/* needed for libcurl function msHTTPGetFile in maphttp.c */
#if defined(USE_WMS_LYR) || defined(USE_WFS_LYR)

    char *pszSLDTmpFile = NULL;
    int status = 0;
    char *pszSLDbuf=NULL;
    FILE *fp = NULL;               
    int nStatus = MS_FAILURE;

    if (map && szURL)
    {
        pszSLDTmpFile = msTmpFile(map->mappath, map->web.imagepath, "sld.xml");
        if (msHTTPGetFile(szURL, pszSLDTmpFile, &status,-1, 0, 0) ==  MS_SUCCESS)
        {
            if ((fp = fopen(pszSLDTmpFile, "rb")) != NULL)
            {
                int   nBufsize=0;
                fseek(fp, 0, SEEK_END);
                nBufsize = ftell(fp);
                rewind(fp);
                pszSLDbuf = (char*)malloc((nBufsize+1)*sizeof(char));
                fread(pszSLDbuf, 1, nBufsize, fp);
                fclose(fp);
                pszSLDbuf[nBufsize] = '\0';
                unlink(pszSLDTmpFile);
            }
        }

        if (pszSLDbuf)
          nStatus = msSLDApplySLD(map, pszSLDbuf, iLayer, pszStyleLayerName);
    }

    return nStatus;

#else
    msSetError(MS_MISCERR, "WMS/WFS client support is not enabled .", "msSLDApplySLDURL()");
    return(MS_FAILURE);
#endif

#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

    msSetError(MS_MISCERR, "OGR support is not available.", "msSLDApplySLDURL()");
    return(MS_FAILURE);

#endif /* USE_OGR */

}

/************************************************************************/
/*                              msSLDApplySLD                           */
/*                                                                      */
/*      Parses the SLD into array of layers. Go through the map and     */
/*      compare the SLD layers and the map layers using the name. If    */
/*      they have the same name, copy the classes asscoaited with       */
/*      the SLD layers onto the map layers.                             */
/************************************************************************/
int msSLDApplySLD(mapObj *map, char *psSLDXML, int iLayer,
                  char *pszStyleLayerName)
{
#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR) || defined(USE_SOS_SVR)

#ifdef USE_OGR

    int nLayers = 0;
    layerObj *pasLayers = NULL;
    int i, j, k, iClass;
    int bUseSpecificLayer = 0;
    int bSuccess =0;
    const char *pszTmp = NULL;
    int bFreeTemplate = 0;
    int nLayerStatus = 0;

    pasLayers = msSLDParseSLD(map, psSLDXML, &nLayers);


    if (pasLayers && nLayers > 0)
    {
        for (i=0; i<map->numlayers; i++)
        {
            if (iLayer >=0 && iLayer< map->numlayers)
            {
                i = iLayer;
                bUseSpecificLayer = 1;
            }

            /* compare layer name to wms_name as well */
            pszTmp = msOWSLookupMetadata(&(map->layers[i].metadata), "MO", "name");

            for (j=0; j<nLayers; j++)
            {
                    
/* -------------------------------------------------------------------- */
/*      copy :  - class                                                 */
/*              - layer's labelitem                                     */
/* -------------------------------------------------------------------- */
                if ((pszStyleLayerName == NULL && 
                     ((strcasecmp(map->layers[i].name, pasLayers[j].name) == 0 ||
                      (pszTmp && strcasecmp(pszTmp, pasLayers[j].name) == 0))||
                     (map->layers[i].group && 
                       strcasecmp(map->layers[i].group, pasLayers[j].name) == 0))) ||
                    (bUseSpecificLayer && pszStyleLayerName && 
                     strcasecmp(pasLayers[j].name, pszStyleLayerName) == 0))
                {
                    bSuccess =1;
/* -------------------------------------------------------------------- */
/*      copy classes in reverse order : the Rule priority is the        */
/*      first rule is the most important (mapserver uses the painter    */
/*      model)                                                          */
/* -------------------------------------------------------------------- */
                    map->layers[i].type = pasLayers[j].type;
                    map->layers[i].numclasses = 0;
                    iClass = 0;
                    for (k=0; k < pasLayers[j].numclasses; k++)
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

                    if (pasLayers[j].classitem)
                    {
                        if (map->layers[i].classitem)
                          free(map->layers[i].classitem);

                        map->layers[i].classitem = strdup(pasLayers[j].classitem);
                    }
                    
                    /* transparency for sld raster (opacity parameter) */
                    if (map->layers[i].type == MS_LAYER_RASTER && 
                        pasLayers[j].transparency != -1)
                      map->layers[i].transparency = pasLayers[j].transparency;

                    /* mark as auto-generate SLD */
                    if (map->layers[i].connectiontype == MS_WMS)
                      msInsertHashTable(&(map->layers[i].metadata), 
                                        "wms_sld_body", "auto" );
/* ==================================================================== */
/*      if the SLD contained a spatial feature, the layerinfo           */
/*      parameter contains the node. Extract it and do a query on       */
/*      the layer. Insert also a metadata that will be used when        */
/*      rendering the final image.                                      */
/* ==================================================================== */
                    if (pasLayers[j].layerinfo && 
                        (map->layers[i].type ==  MS_LAYER_POINT || 
                         map->layers[i].type == MS_LAYER_LINE ||
                         map->layers[i].type == MS_LAYER_POLYGON ||
                         map->layers[i].type == MS_LAYER_ANNOTATION ||
                         map->layers[i].type == MS_LAYER_TILEINDEX))
                    {  
                        FilterEncodingNode *psNode = NULL;

                        msInsertHashTable(&(map->layers[i].metadata), 
                                          "tmp_wms_sld_query", "true" );
                        psNode = (FilterEncodingNode *)pasLayers[j].layerinfo;

/* -------------------------------------------------------------------- */
/*      set the template on the classes so that the query works         */
/*      using classes. If there are no classes, set it at the layer level.*/
/* -------------------------------------------------------------------- */
                        if (map->layers[i].numclasses > 0)
                        {
                            for (k=0; k<map->layers[i].numclasses; k++)
                            {
                                if (!map->layers[i].class[k].template)
                                  map->layers[i].class[k].template = strdup("ttt.html");
                            }
                        }
                        else if (!map->layers[i].template)
                        {
                            bFreeTemplate = 1;
                            map->layers[i].template = strdup("ttt.html");
                        }

                        nLayerStatus =  map->layers[i].status;
                        map->layers[i].status = MS_ON;
                        FLTApplySpatialFilterToLayer(psNode, map,  
                                                     map->layers[i].index);
                        map->layers[i].status = nLayerStatus;
                        FLTFreeFilterEncodingNode(psNode);

                        if ( bFreeTemplate)
                        {
                            free(map->layers[i].template);
                            map->layers[i].template = NULL;
                        }
                    }
                    break;
                }
            }
            if (bUseSpecificLayer)
              break;
          
              
        }

    }

    if (bSuccess)
      return MS_SUCCESS;

    return(MS_FAILURE);
    

#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

    msSetError(MS_MISCERR, "OGR support is not available.", "msSLDApplySLD()");
    return(MS_FAILURE);

#endif /* USE_OGR */

#else
    msSetError(MS_MISCERR, "OWS support is not available.", 
               "msSLDGenerateSLDLayer()");
    return(MS_FAILURE);
#endif
}



#ifdef USE_OGR
                       
    
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
    layerObj *pasLayers = NULL;
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

    /* strip namespaces ogc and sld and gml */
    CPLStripXMLNamespace(psRoot, "ogc", 1); 
    CPLStripXMLNamespace(psRoot, "sld", 1); 
    CPLStripXMLNamespace(psRoot, "gml", 1); 
    

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
    while (psNamedLayer)
    {
        if (!psNamedLayer->pszValue || 
            strcasecmp(psNamedLayer->pszValue, "NamedLayer") != 0)
        {
            psNamedLayer = psNamedLayer->psNext;
            continue;
        }
    
        psNamedLayer = psNamedLayer->psNext;
        nLayers++;
    }

    if (nLayers > 0)
      pasLayers = (layerObj *)malloc(sizeof(layerObj)*nLayers);
    else
      return NULL;

    psNamedLayer = CPLGetXMLNode(psSLD, "NamedLayer");
    if (psNamedLayer)
    {
        iLayer = 0;
        while (psNamedLayer) 

        {
            if (!psNamedLayer->pszValue || 
                strcasecmp(psNamedLayer->pszValue, "NamedLayer") != 0)
            {
                psNamedLayer = psNamedLayer->psNext;
                continue;
            }

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
/*                           _SLDApplyRuleValues                        */
/*                                                                      */
/*      Utility function to set the scale, title/name for the          */
/*      classes created by a Rule.                                      */
/************************************************************************/
void  _SLDApplyRuleValues(CPLXMLNode *psRule, layerObj *psLayer, 
                          int nNewClasses)
{
    int         i=0;
    CPLXMLNode *psMinScale=NULL, *psMaxScale=NULL;
    CPLXMLNode *psName=NULL, *psTitle=NULL;
    double dfMinScale=0, dfMaxScale=0;
    char *pszName=NULL, *pszTitle=NULL;

    if (psRule && psLayer && nNewClasses > 0)
    {
/* -------------------------------------------------------------------- */
/*      parse minscale and maxscale.                                    */
/* -------------------------------------------------------------------- */
        psMinScale = CPLGetXMLNode(psRule, 
                                   "MinScaleDenominator");
        if (psMinScale && psMinScale->psChild && 
            psMinScale->psChild->pszValue)
          dfMinScale = atof(psMinScale->psChild->pszValue);

        psMaxScale = CPLGetXMLNode(psRule, 
                                   "MaxScaleDenominator");
        if (psMaxScale && psMaxScale->psChild && 
            psMaxScale->psChild->pszValue)
          dfMaxScale = atof(psMaxScale->psChild->pszValue);

/* -------------------------------------------------------------------- */
/*      parse name and title.                                           */
/* -------------------------------------------------------------------- */
        psName = CPLGetXMLNode(psRule, "Name");  
        if (psName && psName->psChild && 
            psName->psChild->pszValue)
          pszName = psName->psChild->pszValue;

        psTitle = CPLGetXMLNode(psRule, "Title");  
        if (psTitle && psTitle->psChild && 
            psTitle->psChild->pszValue)
          pszTitle = psTitle->psChild->pszValue;

/* -------------------------------------------------------------------- */
/*      set the scale to all the classes created by the rule.           */
/* -------------------------------------------------------------------- */
        if (dfMinScale > 0 || dfMaxScale > 0)
        {
            for (i=0; i<nNewClasses; i++)
            {
                if (dfMinScale > 0)
                  psLayer->class[psLayer->numclasses-1-i].minscale = dfMinScale;
                if (dfMaxScale)
                  psLayer->class[psLayer->numclasses-1-i].maxscale = dfMaxScale;
            }                           
        }
/* -------------------------------------------------------------------- */
/*      set name and title to the classes created by the rule.          */
/* -------------------------------------------------------------------- */
        for (i=0; i<nNewClasses; i++)
        {
            if (!psLayer->class[psLayer->numclasses-1-i].name)
            {
                if (pszName)
                  psLayer->class[psLayer->numclasses-1-i].name = strdup(pszName);
                else if (pszTitle)
                  psLayer->class[psLayer->numclasses-1-i].name = strdup(pszTitle);
                else
                  psLayer->class[psLayer->numclasses-1-i].name = strdup("Unknown");
            }
        }
        if (pszTitle)
        {
            for (i=0; i<nNewClasses; i++)
            {
                psLayer->class[psLayer->numclasses-1-i].title = 
                  strdup(pszTitle);
            }                           
        }
          
    }
        
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
    CPLXMLNode *psTmpNode = NULL;
    FilterEncodingNode *psNode = NULL;
    char *szExpression = NULL;
    char *szClassItem = NULL;
    int i=0, nNewClasses=0, nClassBeforeFilter=0, nClassAfterFilter=0;
    int nClassAfterRule=0, nClassBeforeRule=0;
    char *pszTmpFilter = NULL;


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
                    if (!psFeatureTypeStyle->pszValue ||
                        strcasecmp(psFeatureTypeStyle->pszValue, 
                                   "FeatureTypeStyle") != 0)
                    {
                        psFeatureTypeStyle = psFeatureTypeStyle->psNext;
                        continue;
                    }

/* -------------------------------------------------------------------- */
/*      Parse rules with no Else filter.                                */
/* -------------------------------------------------------------------- */
                    psRule = CPLGetXMLNode(psFeatureTypeStyle, "Rule");
                    while (psRule)
                    {
                        if (!psRule->pszValue || 
                            strcasecmp(psRule->pszValue, "Rule") != 0)
                        {
                            psRule = psRule->psNext;
                            continue;
                        }
                        /* used for scale setting */
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
                            
                            /* clone the tree and set the next node to null */
                            /* so we only have the Filter node */
                            psTmpNode = CPLCloneXMLTree(psFilter);
                            psTmpNode->psNext = NULL;
                            pszTmpFilter = CPLSerializeXMLTree(psTmpNode);
                            CPLDestroyXMLNode(psTmpNode);

                            if (pszTmpFilter)
                            {
                            /* nTmp = strlen(psFilter->psChild->pszValue)+17; */
                            /* pszTmpFilter = malloc(sizeof(char)*nTmp); */
                            /* sprintf(pszTmpFilter,"<Filter>%s</Filter>", */
                            /* psFilter->psChild->pszValue); */
                            /* pszTmpFilter[nTmp-1]='\0'; */
                                psNode = FLTParseFilterEncoding(pszTmpFilter);
                            
                                CPLFree(pszTmpFilter);
                            }

                            if (psNode)
                            {
/* ==================================================================== */
/*      If the filter has a spatial filter, we keep the node. This      */
/*      node will be parsed when applying the SLD and be used to do     */
/*      queries on the layer.                                           */
/* ==================================================================== */
                                if (FLTHasSpatialFilter(psNode))
                                  psLayer->layerinfo = (void *)psNode;

                                szExpression = FLTGetMapserverExpression(psNode);

                                if (szExpression)
                                {
                                    szClassItem = 
                                      FLTGetMapserverExpressionClassItem(psNode);
                                    nNewClasses = 
                                      nClassAfterFilter - nClassBeforeFilter;
                                    for (i=0; i<nNewClasses; i++)
                                    {
                                        msLoadExpressionString(&psLayer->
                                                             class[psLayer->numclasses-1-i].
                                                             expression, szExpression);
                                    }
                                    if (szClassItem)
                                      psLayer->classitem = strdup(szClassItem);
                                }
                            }
                        }
                        nClassAfterRule = psLayer->numclasses;
                        nNewClasses = nClassAfterRule - nClassBeforeRule;

                        /* apply scale and title to newly created classes */
                        _SLDApplyRuleValues(psRule, psLayer, nNewClasses);

                        /* TODO : parse legendgraphic */
                        psRule = psRule->psNext;

                    }
/* -------------------------------------------------------------------- */
/*      First parse rules with the else filter. These rules will        */
/*      create the classes that are placed at the end of class          */
/*      list. (See how classes are applied to layers in function        */
/*      msSLDApplySLD).                                                 */
/* -------------------------------------------------------------------- */
                    psRule = CPLGetXMLNode(psFeatureTypeStyle, "Rule");
                    while (psRule)
                    {
                        if (!psRule->pszValue || 
                            strcasecmp(psRule->pszValue, "Rule") != 0)
                        {
                            psRule = psRule->psNext;
                            continue;
                        }
                        psElseFilter = CPLGetXMLNode(psRule, "ElseFilter");
                        if (psElseFilter)
                        {
                            msSLDParseRule(psRule, psLayer);
                            _SLDApplyRuleValues(psRule, psLayer, 1);
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
    CPLXMLNode *psRasterSymbolizer = NULL;

    int bSymbolizer = 0;
    int bNewClass=0, nSymbolizer=0;

    if (psRoot && psLayer)
    {
        /* TODO : parse name of the rule */
/* -------------------------------------------------------------------- */
/*      The SLD specs assumes here that a certain FeatureType can only have*/
/*      rules for only one type of symbolizer.                          */
/* -------------------------------------------------------------------- */
/* ==================================================================== */
/*      For each rule a new class is created. If there are more than    */
/*      one symbolizer of the same type, a style is added in the        */
/*      same class.                                                     */
/* ==================================================================== */
        nSymbolizer =0;
 
        /* line symbolizer */
        psLineSymbolizer = CPLGetXMLNode(psRoot, "LineSymbolizer");
        while (psLineSymbolizer)
        {
            if (!psLineSymbolizer->pszValue || 
                strcasecmp(psLineSymbolizer->pszValue, 
                           "LineSymbolizer") != 0)
            {
                psLineSymbolizer = psLineSymbolizer->psNext;
                continue;
            }

            bSymbolizer = 1;
            if (nSymbolizer == 0)
              bNewClass = 1;
            else
              bNewClass = 0;

            msSLDParseLineSymbolizer(psLineSymbolizer, psLayer, bNewClass);
            psLineSymbolizer = psLineSymbolizer->psNext;
            psLayer->type = MS_LAYER_LINE;
            nSymbolizer++;
        }

        /* Polygon symbolizer */
        psPolygonSymbolizer = CPLGetXMLNode(psRoot, "PolygonSymbolizer");
        while (psPolygonSymbolizer)        
        {
            if (!psPolygonSymbolizer->pszValue || 
                strcasecmp(psPolygonSymbolizer->pszValue, 
                           "PolygonSymbolizer") != 0)
            {
                psPolygonSymbolizer = psPolygonSymbolizer->psNext;
                continue;
            }
            bSymbolizer = 1;
            if (nSymbolizer == 0)
              bNewClass = 1;
            else
              bNewClass = 0;
            msSLDParsePolygonSymbolizer(psPolygonSymbolizer, psLayer,
                                        bNewClass);
            psPolygonSymbolizer = psPolygonSymbolizer->psNext;
            psLayer->type = MS_LAYER_POLYGON;
            nSymbolizer++;
        }
        /* Point Symbolizer */
        psPointSymbolizer = CPLGetXMLNode(psRoot, "PointSymbolizer");
        while (psPointSymbolizer)
        {
            if (!psPointSymbolizer->pszValue || 
                strcasecmp(psPointSymbolizer->pszValue, 
                           "PointSymbolizer") != 0)
            {
                psPointSymbolizer = psPointSymbolizer->psNext;
                continue;
            }
            bSymbolizer = 1;
            if (nSymbolizer == 0)
              bNewClass = 1;
            else
              bNewClass = 0;
            msSLDParsePointSymbolizer(psPointSymbolizer, psLayer, bNewClass);
            psPointSymbolizer = psPointSymbolizer->psNext;
            psLayer->type = MS_LAYER_POINT;
            nSymbolizer++;
        }
        /* Text symbolizer */
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
            if (!psTextSymbolizer->pszValue || 
                strcasecmp(psTextSymbolizer->pszValue, 
                           "TextSymbolizer") != 0)
            {
                psTextSymbolizer = psTextSymbolizer->psNext;
                continue;
            }
            if (nSymbolizer == 0)
                psLayer->type = MS_LAYER_ANNOTATION;
            msSLDParseTextSymbolizer(psTextSymbolizer, psLayer, bSymbolizer);
            psTextSymbolizer = psTextSymbolizer->psNext;
        }

        /* Raster symbolizer */
        psRasterSymbolizer = CPLGetXMLNode(psRoot, "RasterSymbolizer");
        while (psRasterSymbolizer && psRasterSymbolizer->pszValue && 
               strcasecmp(psRasterSymbolizer->pszValue, 
                          "RasterSymbolizer") == 0)
        {
            if (!psRasterSymbolizer->pszValue || 
                strcasecmp(psRasterSymbolizer->pszValue, 
                           "RasterSymbolizer") != 0)
            {
                psRasterSymbolizer = psRasterSymbolizer->psNext;
                continue;
            }
            msSLDParseRasterSymbolizer(psRasterSymbolizer, psLayer);
            psRasterSymbolizer = psRasterSymbolizer->psNext;
            psLayer->type = MS_LAYER_RASTER;
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
void msSLDParseLineSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer,
                              int bNewClass)
{
    int nClassId = 0;
    CPLXMLNode *psStroke;
    int iStyle = 0;

    if (psRoot && psLayer)
    {
        psStroke =  CPLGetXMLNode(psRoot, "Stroke");
        if (psStroke)
        {
            if (bNewClass || psLayer->numclasses <= 0)
            {
                initClass(&(psLayer->class[psLayer->numclasses]));
                nClassId = psLayer->numclasses;
                psLayer->numclasses++;
            }
            else
              nClassId = psLayer->numclasses-1;

            iStyle = psLayer->class[nClassId].numstyles;
            initStyle(&(psLayer->class[nClassId].styles[iStyle]));
            psLayer->class[nClassId].numstyles++;
            
            msSLDParseStroke(psStroke, &psLayer->class[nClassId].styles[iStyle],
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
        /* parse css parameters */
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
                        /* expecting hexadecimal ex : #aaaaff */
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
                                
                        /* use an ellipse symbol for the width */
                        if (psStyle->symbol <=0)
                        {
                            psStyle->symbol = 
                              msSLDGetLineSymbol(map);
                            if (psStyle->symbol > 0 &&
                                psStyle->symbol < map->symbolset.numsymbols)
                              psStyle->symbolname = 
                                strdup(map->symbolset.symbol[psStyle->symbol].name);
                        }
                    }
                }
                else if (strcasecmp(psStrkName, "stroke-dasharray") == 0)
                {
                    if(psCssParam->psChild && psCssParam->psChild->psNext &&
                       psCssParam->psChild->psNext->pszValue)
                    {
                        pszDashValue = 
                          strdup(psCssParam->psChild->psNext->pszValue);
                        /* use an ellipse symbol with dash arrays */
                        psStyle->symbol = 
                          msSLDGetDashLineSymbol(map, 
                                                 psCssParam->psChild->psNext->pszValue);
                        if ( psStyle->symbol > 0 && 
                             psStyle->symbol < map->symbolset.numsymbols)
                          psStyle->symbolname = 
                            strdup(map->symbolset.symbol[psStyle->symbol].name);
                    }
                }
            }
            psCssParam = psCssParam->psNext;
        }

        /* parse graphic fill or stroke */
        /* graphic fill and graphic stroke pare parsed the same way :  */
        /* TODO : It seems inconsistent to me since the only diffrence */
        /* between them seems to be fill (fill) or not fill (stroke). And */
        /* then again the fill parameter can be used inside both elements. */
        psGraphicFill =  CPLGetXMLNode(psStroke, "GraphicFill");
        if (psGraphicFill)
          msSLDParseGraphicFillOrStroke(psGraphicFill, pszDashValue, psStyle, map, 0);
        psGraphicFill =  CPLGetXMLNode(psStroke, "GraphicStroke");
        if (psGraphicFill)
          msSLDParseGraphicFillOrStroke(psGraphicFill, pszDashValue, psStyle, map, 0);

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
void msSLDParsePolygonSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer, 
                                 int bNewClass)
{
    CPLXMLNode *psFill, *psStroke;
    int nClassId=0, iStyle=0;

    if (psRoot && psLayer)
    {
        psFill =  CPLGetXMLNode(psRoot, "Fill");
        if (psFill)
        {
            if (bNewClass || psLayer->numclasses <= 0)
            {
                initClass(&(psLayer->class[psLayer->numclasses]));
                nClassId = psLayer->numclasses;
                psLayer->numclasses++;
            }
            else
               nClassId = psLayer->numclasses-1;

            iStyle = psLayer->class[nClassId].numstyles;
            initStyle(&(psLayer->class[nClassId].styles[iStyle]));
            psLayer->class[nClassId].numstyles++;
            
            msSLDParsePolygonFill(psFill, &psLayer->class[nClassId].styles[iStyle],
                                  psLayer->map);
        }
        /* stroke wich corresponds to the outilne in mapserver */
        /* is drawn after the fill */
        psStroke =  CPLGetXMLNode(psRoot, "Stroke");
        if (psStroke)
        {
/* -------------------------------------------------------------------- */
/*      there was a fill so add a style to the last class created       */
/*      by the fill                                                     */
/* -------------------------------------------------------------------- */
            if (psFill && psLayer->numclasses > 0) 
            {
                nClassId =psLayer->numclasses-1;
                iStyle = psLayer->class[nClassId].numstyles;
                initStyle(&(psLayer->class[nClassId].styles[iStyle]));
                psLayer->class[nClassId].numstyles++;
            }
            else
            {
                if (bNewClass || psLayer->numclasses <= 0)
                {
                    initClass(&(psLayer->class[psLayer->numclasses]));
                    nClassId = psLayer->numclasses;
                    psLayer->numclasses++;
                }
                else
                  nClassId = psLayer->numclasses-1;

                iStyle = psLayer->class[nClassId].numstyles;
                initStyle(&(psLayer->class[nClassId].styles[iStyle]));
                psLayer->class[nClassId].numstyles++;
                
            }
            msSLDParseStroke(psStroke, &psLayer->class[nClassId].styles[iStyle],
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
        /* sets the default fill color defined in the spec #808080 */
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
                        /* expecting hexadecimal ex : #aaaaff */
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
        
        /* graphic fill and graphic stroke pare parsed the same way :  */
        /* TODO : It seems inconsistent to me since the only diffrence */
        /* between them seems to be fill (fill) or not fill (stroke). And */
        /* then again the fill parameter can be used inside both elements. */
        psGraphicFill =  CPLGetXMLNode(psFill, "GraphicFill");
        if (psGraphicFill)
          msSLDParseGraphicFillOrStroke(psGraphicFill, NULL, psStyle, map, 0);
        psGraphicFill =  CPLGetXMLNode(psFill, "GraphicStroke");
        if (psGraphicFill)
          msSLDParseGraphicFillOrStroke(psGraphicFill, NULL, psStyle, map, 0);

        
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
                                   styleObj *psStyle, mapObj *map,
                                   int bPointLayer)
{
    CPLXMLNode  *psCssParam, *psGraphic, *psExternalGraphic, *psMark, *psSize;
    CPLXMLNode *psWellKnownName, *psStroke, *psFill;
    char *psColor=NULL, *psColorName = NULL;
    int nLength = 0;
    char *pszSymbolName = NULL;
    int bFilled = 0, bStroked=0; 

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
            /* extract symbol size */
            psSize = CPLGetXMLNode(psGraphic, "Size");
            if (psSize && psSize->psChild && psSize->psChild->pszValue)
              psStyle->size = atoi(psSize->psChild->pszValue);
            else
              psStyle->size = 6; /* defualt value */

            /* extract symbol */
            psMark =  CPLGetXMLNode(psGraphic, "Mark");
            if (psMark)
            {
                pszSymbolName = NULL;
                psWellKnownName =  CPLGetXMLNode(psMark, "WellKnownName");
                if (psWellKnownName && psWellKnownName->psChild &&
                    psWellKnownName->psChild->pszValue)
                  pszSymbolName = 
                    strdup(psWellKnownName->psChild->pszValue);
                    
                /* default symbol is square */
                
                if (!pszSymbolName || 
                    (strcasecmp(pszSymbolName, "square") != 0 &&
                     strcasecmp(pszSymbolName, "circle") != 0 &&
                     strcasecmp(pszSymbolName, "triangle") != 0 &&
                     strcasecmp(pszSymbolName, "star") != 0 &&
                     strcasecmp(pszSymbolName, "cross") != 0 &&
                     strcasecmp(pszSymbolName, "x") != 0))
                {
                    if (msGetSymbolIndex(&map->symbolset, pszSymbolName,  MS_FALSE) < 0)
                      pszSymbolName = strdup("square");
                }
                
                
                /* check if the symbol should be filled or not */
                psFill = CPLGetXMLNode(psMark, "Fill");
                psStroke = CPLGetXMLNode(psMark, "Stroke");
                    
                if (psFill || psStroke)
                {
                    if (psFill)
                      bFilled = 1;
                    else
                      bFilled = 0;

                    if (psStroke)
                      bStroked = 1;
                    else
                      bStroked = 0;

                    if (psFill)
                    {
                        psCssParam =  CPLGetXMLNode(psFill, "CssParameter");
                        while (psCssParam && psCssParam->pszValue && 
                               strcasecmp(psCssParam->pszValue, "CssParameter") == 0)
                        {
                            psColorName = 
                              (char*)CPLGetXMLValue(psCssParam, "name", NULL);
                            if (psColorName && 
                                strcasecmp(psColorName, "fill") == 0)
                            {
                                if(psCssParam->psChild && 
                                   psCssParam->psChild->psNext && 
                                   psCssParam->psChild->psNext->pszValue)
                                  psColor = psCssParam->psChild->psNext->pszValue;

                                if (psColor)
                                {
                                    nLength = strlen(psColor);
                                    if (nLength == 7 && psColor[0] == '#')
                                    {
                                        msSLDSetColorObject(psColor,
                                                            &psStyle->color);
                                    }
                                }
                                break;
                            }
                        
                            psCssParam = psCssParam->psNext;
                        }
                    }
                    if (psStroke)
                    {
                        psCssParam =  CPLGetXMLNode(psStroke, "CssParameter");
                        while (psCssParam && psCssParam->pszValue && 
                               strcasecmp(psCssParam->pszValue, "CssParameter") == 0)
                        {
                            psColorName = 
                              (char*)CPLGetXMLValue(psCssParam, "name", NULL);
                            if (psColorName && 
                                strcasecmp(psColorName, "stroke") == 0) 
                            {
                                if(psCssParam->psChild && 
                                   psCssParam->psChild->psNext && 
                                   psCssParam->psChild->psNext->pszValue)
                                  psColor = psCssParam->psChild->psNext->pszValue;

                                if (psColor)
                                {
                                    nLength = strlen(psColor);
                                    if (nLength == 7 && psColor[0] == '#')
                                    {
                                      msSLDSetColorObject(psColor,
                                                          &psStyle->outlinecolor);
                                    }
                                }
                                break;
                            }
                        
                            psCssParam = psCssParam->psNext;
                        }
                    }
                    

                     /* set the default color if color is not not already set */
                    if ((psStyle->color.red < 0 || 
                        psStyle->color.green == -1 ||
                         psStyle->color.blue == -1) &&
                        (psStyle->outlinecolor.red == -1 ||
                         psStyle->outlinecolor.green == -1 ||
                         psStyle->outlinecolor.blue == -1))
                    {
                        psStyle->color.red = 128;
                        psStyle->color.green = 128;
                        psStyle->color.blue = 128;
                    }
                    
                }
                /* Get the corresponding symbol id  */
                psStyle->symbol = msSLDGetMarkSymbol(map, pszSymbolName, 
                                                     bFilled, pszDashValue);
                if (psStyle->symbol > 0 &&
                    psStyle->symbol < map->symbolset.numsymbols)
                  psStyle->symbolname = 
                    strdup(map->symbolset.symbol[psStyle->symbol].name);
                    
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
    nSymbolId = msGetSymbolIndex(&map->symbolset, SLD_LINE_SYMBOL_NAME, MS_FALSE);
    if (nSymbolId >= 0)
      return nSymbolId;

    if(map->symbolset.numsymbols == MS_MAXSYMBOLS) 
    { 
	msSetError(MS_SYMERR, "Too many symbols defined.", "msSLDGetLineSymbol()");
        return 0; /* returs 0 for no symbol */
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
    symbolObj *psSymbol = NULL;
    char **aszValues = NULL;
    int nDash, i;


    if(map->symbolset.numsymbols == MS_MAXSYMBOLS) 
    { 
	msSetError(MS_SYMERR, "Too many symbols defined.", "msSLDGetDashLineSymbol()");
        return 0; /* returs 0 for no symbol */
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

    psSymbol->name = strdup(SLD_LINE_SYMBOL_DASH_NAME);
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
                                       SLD_MARK_SYMBOL_SQUARE_FILLED, 
                                       MS_FALSE);
        else
          nSymbolId = msGetSymbolIndex(&map->symbolset, 
                                       SLD_MARK_SYMBOL_SQUARE, 
                                       MS_FALSE);
    }   
    else if (strcasecmp(pszSymbolName, "circle") == 0)
    {
        
        if (bFilled)
          nSymbolId = msGetSymbolIndex(&map->symbolset, 
                                       SLD_MARK_SYMBOL_CIRCLE_FILLED, 
                                       MS_FALSE);
        else
          nSymbolId = msGetSymbolIndex(&map->symbolset, 
                                       SLD_MARK_SYMBOL_CIRCLE, 
                                       MS_FALSE);
    }
    else if (strcasecmp(pszSymbolName, "triangle") == 0)
    {
        
        if (bFilled)
          nSymbolId = msGetSymbolIndex(&map->symbolset, 
                                       SLD_MARK_SYMBOL_TRIANGLE_FILLED, 
                                       MS_FALSE);
        else
          nSymbolId = msGetSymbolIndex(&map->symbolset, 
                                       SLD_MARK_SYMBOL_TRIANGLE, 
                                       MS_FALSE);
    }
    else if (strcasecmp(pszSymbolName, "star") == 0)
    {
        
        if (bFilled)
          nSymbolId = msGetSymbolIndex(&map->symbolset, 
                                       SLD_MARK_SYMBOL_STAR_FILLED, 
                                       MS_FALSE);
        else
          nSymbolId = msGetSymbolIndex(&map->symbolset, 
                                       SLD_MARK_SYMBOL_STAR, 
                                       MS_FALSE);
    }
    else if (strcasecmp(pszSymbolName, "cross") == 0)
    {
        
        if (bFilled)
          nSymbolId = msGetSymbolIndex(&map->symbolset, 
                                       SLD_MARK_SYMBOL_CROSS_FILLED, 
                                       MS_FALSE);
        else
          nSymbolId = msGetSymbolIndex(&map->symbolset, 
                                       SLD_MARK_SYMBOL_CROSS, 
                                       MS_FALSE);
    }
    else if (strcasecmp(pszSymbolName, "x") == 0)
    {
        
        if (bFilled)
          nSymbolId = msGetSymbolIndex(&map->symbolset, 
                                       SLD_MARK_SYMBOL_X_FILLED, 
                                       MS_FALSE);
        else
          nSymbolId = msGetSymbolIndex(&map->symbolset, 
                                       SLD_MARK_SYMBOL_X, 
                                       MS_FALSE);
    }
    else
    {
        nSymbolId = msGetSymbolIndex(&map->symbolset, 
                                     pszSymbolName, 
                                     MS_FALSE);
    }

    if (nSymbolId <= 0)
    {
        if(map->symbolset.numsymbols == MS_MAXSYMBOLS) 
        {    
            msSetError(MS_SYMERR, "Too many symbols defined.", 
                       "msSLDGetMarkSymbol()");
            return 0; /* returs 0 for no symbol */
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
        /* cross is like plus (+) since there is also X symbol ?? */
        else if (strcasecmp(pszSymbolName, "cross") == 0)
        {
            /* NEVER FILL CROSS */
            /* if (bFilled) */
            /* psSymbol->name = strdup(SLD_MARK_SYMBOL_CROSS_FILLED); */
            /* else */
              psSymbol->name = strdup(SLD_MARK_SYMBOL_CROSS);
            
            psSymbol->type = MS_SYMBOL_VECTOR;
            /* if (bFilled) */
            /* psSymbol->filled = MS_TRUE; */

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
            /* NEVER FILL X */
            /* if (bFilled) */
            /* psSymbol->name = strdup(SLD_MARK_SYMBOL_X_FILLED); */
            /* else */
              psSymbol->name = strdup(SLD_MARK_SYMBOL_X);
            
            psSymbol->type = MS_SYMBOL_VECTOR;
            /* if (bFilled) */
            /* psSymbol->filled = MS_TRUE; */
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
        /* check if a symbol of a  */
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
                psSymbol->name = strdup(pszFileName);

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
void msSLDParsePointSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer,
                               int bNewClass)
{
    int nClassId = 0;
    int iStyle = 0;

    if (psRoot && psLayer)
    {
        if (bNewClass || psLayer->numclasses <= 0)
        {
            initClass(&(psLayer->class[psLayer->numclasses]));
            nClassId = psLayer->numclasses;
            psLayer->numclasses++;
        }
        else
          nClassId = psLayer->numclasses-1;

        iStyle = psLayer->class[nClassId].numstyles;
        initStyle(&(psLayer->class[nClassId].styles[iStyle]));
        psLayer->class[nClassId].numstyles++;
        

        /* set the default color */
        psLayer->class[nClassId].styles[iStyle].color.red = 128;
        psLayer->class[nClassId].styles[iStyle].color.green = 128;
        psLayer->class[nClassId].styles[iStyle].color.blue = 128;

        msSLDParseGraphicFillOrStroke(psRoot, NULL,
                                      &psLayer->class[nClassId].styles[iStyle],
                                      psLayer->map, 1);
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
/* needed for libcurl function msHTTPGetFile in maphttp.c */
#if defined(USE_WMS_LYR) || defined(USE_WFS_LYR)

    char *pszFormat = NULL;
    CPLXMLNode *psURL=NULL, *psFormat=NULL, *psTmp=NULL;
    char *pszURL=NULL, *pszTmpSymbolName=NULL;
    int status;

    if (psExternalGraphic && psStyle && map)
    {
        psFormat = CPLGetXMLNode(psExternalGraphic, "Format");
        if (psFormat && psFormat->psChild && psFormat->psChild->pszValue)
          pszFormat = psFormat->psChild->pszValue;

        /* supports GIF and PNG */
        if (pszFormat && 
            (strcasecmp(pszFormat, "GIF") == 0 ||
             strcasecmp(pszFormat, "image/gif") == 0 || 
             strcasecmp(pszFormat, "PNG") == 0 ||
             strcasecmp(pszFormat, "image/png") == 0))
        {
          
          /* <OnlineResource xmlns:xlink="http://www.w3.org/1999/xlink" xlink:type="simple" xlink:href="http://www.vendor.com/geosym/2267.svg"/> */
            psURL = CPLGetXMLNode(psExternalGraphic, "OnlineResource");
            if (psURL && psURL->psChild)
            {
                psTmp =  psURL->psChild;
                while (psTmp != NULL && 
                       psTmp->pszValue && 
                       strcasecmp(psTmp->pszValue, "xlink:href") != 0)
                {
                    psTmp = psTmp->psNext;
                }
                if (psTmp && psTmp->psChild)
                {
                    pszURL = (char*)psTmp->psChild->pszValue;

                    if (strcasecmp(pszFormat, "GIF") == 0 || 
                        strcasecmp(pszFormat, "image/gif") == 0)
                      pszTmpSymbolName = msTmpFile(map->mappath, map->web.imagepath, "gif");
                    else
                      pszTmpSymbolName = msTmpFile(map->mappath, map->web.imagepath, "png");

                    if (msHTTPGetFile(pszURL, pszTmpSymbolName, &status,-1, 0, 0) ==  
                        MS_SUCCESS)
                    {
                        psStyle->symbol = msSLDGetGraphicSymbol(map, pszTmpSymbolName);
                        if (psStyle->symbol > 0 &&
                            psStyle->symbol < map->symbolset.numsymbols)
                          psStyle->symbolname = 
                            strdup(map->symbolset.symbol[psStyle->symbol].name);

                        /* set the color parameter if not set. Does not make sense */
                        /* for pixmap but mapserver needs it. */
                        if (psStyle->color.red == -1 || psStyle->color.green ||
                            psStyle->color.blue)
                        {
                            psStyle->color.red = 0;
                            psStyle->color.green = 0;
                            psStyle->color.blue = 0;
                        }
                     
                    }
                }
            }
        }
    }
#endif
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
            if (nClassId >= 0)/* should always be true */
              nStyleId = psLayer->class[nClassId].numstyles -1;
        }

        if (nStyleId >= 0 && nClassId >= 0) /* should always be true */
          msSLDParseTextParams(psRoot, psLayer,
                               &psLayer->class[nClassId]);
    }
}



/************************************************************************/
/*                        msSLDParseRasterSymbolizer                    */
/*                                                                      */
/*      Supports the ColorMap parameter in a Raster Symbolizer. In      */
/*      the ColorMap, only color and quantity are used here.            */
/*                                                                      */
/*      <xs:element name="RasterSymbolizer">                            */
/*      <xs:complexType>                                                */
/*      <xs:sequence>                                                   */
/*      <xs:element ref="sld:Geometry" minOccurs="0"/>                  */
/*      <xs:element ref="sld:Opacity" minOccurs="0"/>                   */
/*      <xs:element ref="sld:ChannelSelection" minOccurs="0"/>          */
/*      <xs:element ref="sld:OverlapBehavior" minOccurs="0"/>           */
/*      <xs:element ref="sld:ColorMap" minOccurs="0"/>                  */
/*      <xs:element ref="sld:ContrastEnhancement" minOccurs="0"/>       */
/*      <xs:element ref="sld:ShadedRelief" minOccurs="0"/>              */
/*      <xs:element ref="sld:ImageOutline" minOccurs="0"/>              */
/*      </xs:sequence>                                                  */
/*      </xs:complexType>                                               */
/*      </xs:element>                                                   */
/*                                                                      */
/*      <xs:element name="ColorMap">                                    */
/*      <xs:complexType>                                                */
/*      <xs:choice minOccurs="0" maxOccurs="unbounded">                 */
/*      <xs:element ref="sld:ColorMapEntry"/>                           */
/*      </xs:choice>                                                    */
/*      </xs:complexType>                                               */
/*      </xs:element>                                                   */
/*      <xs:element name="ColorMapEntry">                               */
/*      <xs:complexType>                                                */
/*      <xs:attribute name="color" type="xs:string" use="required"/>    */
/*      <xs:attribute name="opacity" type="xs:double"/>                 */
/*      <xs:attribute name="quantity" type="xs:double"/>                */
/*      <xs:attribute name="label" type="xs:string"/>                   */
/*      </xs:complexType>                                               */
/*      </xs:element>                                                   */
/************************************************************************/
void msSLDParseRasterSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer)
{
    CPLXMLNode  *psColorMap = NULL, *psColorEntry = NULL, *psOpacity=NULL;
    char *pszColor=NULL, *pszQuantity=NULL;
    char *pszPreviousColor=NULL, *pszPreviousQuality=NULL;
    colorObj sColor;
    char szExpression[100];
    int nClassId = 0;
    double dfOpacity = 1.0;
    char *pszLabel = NULL;
    char *pch = NULL, *pchPrevious=NULL;

    if (!psRoot || !psLayer)
      return;

/* ==================================================================== */
/*      The defulat transparency value is 0 : we set it here to -1      */
/*      so that when testing the values in msSLDApplySLD (to be         */
/*      applied on the layer), we can assume that a value of 0 comes    */
/*      from the sld.                                                   */
/* ==================================================================== */
    psLayer->transparency = -1;

    psOpacity = CPLGetXMLNode(psRoot, "Opacity");
    if (psOpacity)
    {
        if (psOpacity->psChild && psOpacity->psChild->pszValue)
          dfOpacity = atof(psOpacity->psChild->pszValue);
        
        /* values in sld goes from 0.0 (for transparent) to 1.0 (for full opacity); */
        if (dfOpacity >=0.0 && dfOpacity <=1.0)
          psLayer->transparency = (int)(dfOpacity * 100);
        else
        {
            msSetError(MS_WMSERR, "Invalid opacity value. Values should be between 0.0 and 1.0", "msSLDParseRasterSymbolizer()");
        }
    }
    psColorMap = CPLGetXMLNode(psRoot, "ColorMap");
    if (psColorMap)
    {
        psColorEntry = CPLGetXMLNode(psColorMap, "ColorMapEntry");
        while (psColorEntry && psColorEntry->pszValue &&
               strcasecmp(psColorEntry->pszValue, "ColorMapEntry") == 0)
        {
            pszColor = (char *)CPLGetXMLValue(psColorEntry, "color", NULL);
            pszQuantity = (char *)CPLGetXMLValue(psColorEntry, "quantity", NULL);
            pszLabel = (char *)CPLGetXMLValue(psColorEntry, "label", NULL);

            if (pszColor && pszQuantity)
            {
                if (pszPreviousColor && pszPreviousQuality)
                {
                    if (strlen(pszPreviousColor) == 7 && 
                        pszPreviousColor[0] == '#' &&
                        strlen(pszColor) == 7 && pszColor[0] == '#')
                    {
                        sColor.red = hex2int(pszPreviousColor+1);
                        sColor.green= hex2int(pszPreviousColor+3);
                        sColor.blue = hex2int(pszPreviousColor+5);

                        /* pszQuantity and pszPreviousQuality may be integer or float */
			pchPrevious=strchr(pszPreviousQuality,'.');
			pch=strchr(pszQuantity,'.');
			if (pchPrevious==NULL && pch==NULL) {
			  sprintf(szExpression,
				  "([pixel] >= %d AND [pixel] < %d)",
				  atoi(pszPreviousQuality),
				  atoi(pszQuantity));
			} else if (pchPrevious != NULL && pch==NULL) {
			  sprintf(szExpression,
				  "([pixel] >= %f AND [pixel] < %d)",
				  atof(pszPreviousQuality),
				  atoi(pszQuantity));
			} else if (pchPrevious == NULL && pch != NULL) {
			  sprintf(szExpression,
				  "([pixel] >= %d AND [pixel] < %f)",
				  atoi(pszPreviousQuality),
				  atof(pszQuantity));
			} else {
			  sprintf(szExpression,
				  "([pixel] >= %f AND [pixel] < %f)",
				  atof(pszPreviousQuality),
				  atof(pszQuantity));
			}


                        if (psLayer->numclasses < MS_MAXCLASSES)
                        {
                            initClass(&(psLayer->class[psLayer->numclasses]));
                            psLayer->numclasses++;
                            nClassId = psLayer->numclasses-1;

                            /*set the class name using the label. If label not defined
                              set it with the quantity*/
                            if (pszLabel)
                              psLayer->class[nClassId].name = strdup(pszLabel);
                            else
                              psLayer->class[nClassId].name = strdup(pszQuantity);

                            initStyle(&(psLayer->class[nClassId].styles[0]));
                            psLayer->class[nClassId].numstyles = 1;

                            psLayer->class[nClassId].styles[0].color.red = 
                              sColor.red;
                            psLayer->class[nClassId].styles[0].color.green = 
                              sColor.green;
                            psLayer->class[nClassId].styles[0].color.blue = 
                              sColor.blue;

                            if (psLayer->classitem && 
                                strcasecmp(psLayer->classitem, "[pixel]") != 0)
                              free(psLayer->classitem);
                            psLayer->classitem = strdup("[pixel]");

                            msLoadExpressionString(&psLayer->class[nClassId].expression,
                                                 szExpression);
                            
                            
                        }
                    }
                    else
                    {
                        msSetError(MS_WMSERR, 
                                   "Invalid ColorMap Entry.", 
                                   "msSLDParseRasterSymbolizer()");
                    }

                }
               
                pszPreviousColor = pszColor;
                pszPreviousQuality = pszQuantity;
                
            }
            psColorEntry = psColorEntry->psNext;
        }
        /* do the last Color Map Entry */
        if (pszColor && pszQuantity)
        {
            if (strlen(pszColor) == 7 && pszColor[0] == '#')
            {
                sColor.red = hex2int(pszColor+1);
                sColor.green= hex2int(pszColor+3);
                sColor.blue = hex2int(pszColor+5);

		/* pszQuantity may be integer or float */
		pch=strchr(pszQuantity,'.');
		if (pch==NULL) {
		  sprintf(szExpression, "([pixel] = %d)", atoi(pszQuantity));
		} else {
		  sprintf(szExpression, "([pixel] = %f)", atof(pszQuantity));
		}

                if (psLayer->numclasses < MS_MAXCLASSES)
                {
                    initClass(&(psLayer->class[psLayer->numclasses]));
                    psLayer->numclasses++;
                    nClassId = psLayer->numclasses-1;
                    initStyle(&(psLayer->class[nClassId].styles[0]));
                    psLayer->class[nClassId].numstyles = 1;
                    psLayer->class[nClassId].styles[0].color.red = 
                      sColor.red;
                    psLayer->class[nClassId].styles[0].color.green = 
                              sColor.green;
                    psLayer->class[nClassId].styles[0].color.blue = 
                      sColor.blue;

                    if (psLayer->classitem && 
                        strcasecmp(psLayer->classitem, "[pixel]") != 0)
                      free(psLayer->classitem);
                    psLayer->classitem = strdup("[pixel]");

                    msLoadExpressionString(&psLayer->class[nClassId].expression,
                                         szExpression);
                }
            }
        }
    }
}
/************************************************************************/
/*                           msSLDParseTextParams                       */
/*                                                                      */
/*      Parse text paramaters like font, placement and color.           */
/************************************************************************/
void msSLDParseTextParams(CPLXMLNode *psRoot, layerObj *psLayer, 
                          classObj *psClass)
{
    char szFontName[100];
    int  nFontSize = 10;
    int bFontSet = 0;

    CPLXMLNode *psLabel=NULL, *psFont=NULL;
    CPLXMLNode *psCssParam = NULL;
    char *pszName=NULL, *pszFontFamily=NULL, *pszFontStyle=NULL;
    char *pszFontWeight=NULL; 
    CPLXMLNode *psLabelPlacement=NULL, *psPointPlacement=NULL, *psLinePlacement=NULL;
    CPLXMLNode *psFill = NULL, *psPropertyName=NULL;
    int nLength = 0;
    char *pszColor = NULL;
    char *pszItem = NULL;

    szFontName[0]='\0';

    if (psRoot && psClass && psLayer)
    {
        /* label  */
        /* support literal expression  and  propertyname 
         - <TextSymbolizer><Label>MY_COLUMN</Label>
         - <TextSymbolizer><Label><ogc:PropertyName>MY_COLUMN</ogc:PropertyName></Label>
        Bug 1857 */
        psLabel = CPLGetXMLNode(psRoot, "Label");
        if (psLabel )
        {
            psPropertyName = CPLGetXMLNode(psLabel, "PropertyName");
            if (psPropertyName && psPropertyName->psChild &&
                psPropertyName->psChild->pszValue)
              pszItem = psPropertyName->psChild->pszValue;
            else if (psLabel->psChild && psLabel->psChild->pszValue)
              pszItem = psLabel->psChild->pszValue;

            if (pszItem)
            /* psPropertyName = CPLGetXMLNode(psLabel, "PropertyName"); */
              /* if (psPropertyName && psPropertyName->psChild && */
              /* psPropertyName->psChild->pszValue) */
            {
                if (psLayer->labelitem)
                  free (psLayer->labelitem);
                psLayer->labelitem = strdup(pszItem);

                /* font */
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
                            /* normal, italic, oblique */
                            else if (strcasecmp(pszName, "font-style") == 0)
                            {
                                if(psCssParam->psChild && psCssParam->psChild->psNext && 
                                   psCssParam->psChild->psNext->pszValue)
                                  pszFontStyle = psCssParam->psChild->psNext->pszValue;
                            }
                            /* normal or bold */
                            else if (strcasecmp(pszName, "font-weight") == 0)
                            {
                                 if(psCssParam->psChild && psCssParam->psChild->psNext && 
                                   psCssParam->psChild->psNext->pszValue)
                                  pszFontWeight = psCssParam->psChild->psNext->pszValue;
                            }
                            /* default is 10 pix */
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
                    if (pszFontWeight && strcasecmp(pszFontWeight, "normal") != 0)
                    {
                        strcat(szFontName, "-");
                        strcat(szFontName, pszFontWeight);
                    }
                    if (pszFontStyle && strcasecmp(pszFontStyle, "normal") != 0)
                    {
                        strcat(szFontName, "-");
                        strcat(szFontName, pszFontStyle);
                    }
                        
                    if ((msLookupHashTable(&(psLayer->map->fontset.fonts), szFontName) !=NULL))
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
                                    /* expecting hexadecimal ex : #aaaaff */
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
            
            }/* labelitem */
        }
        /* TODO : support Halo parameter => shadow */
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
        /* init the label with the default position */
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
            /* psCssParam->psChild->psNext->pszValue) */
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
            /* psCssParam->psChild->psNext->pszValue) */
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
    if (psHexColor && psColor && strlen(psHexColor)== 7 && 
        psHexColor[0] == '#')
    {
        
        psColor->red = hex2int(psHexColor+1);
        psColor->green = hex2int(psHexColor+3);
        psColor->blue= hex2int(psHexColor+5);
    }
}   

#endif

/* -------------------------------------------------------------------- */
/*      client sld support functions                                    */
/* -------------------------------------------------------------------- */

/************************************************************************/
/*                msSLDGenerateSLD(mapObj *map, int iLayer)             */
/*                                                                      */
/*      Return an SLD document for all layers that are on or            */
/*      default. The second argument should be set to -1 to genarte     */
/*      on all layers. Or set to the layer index to generate an SLD     */
/*      for a specific layer.                                           */
/*                                                                      */
/*      The caller should free the returned string.                     */
/************************************************************************/
char *msSLDGenerateSLD(mapObj *map, int iLayer)
{
#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR) || defined(USE_SOS_SVR)

#ifdef USE_OGR


    char szTmp[500];
    int i = 0;
    char *pszTmp = NULL;
    char *pszSLD = NULL;
    char *schemalocation = NULL;

    if (map)
    {
        schemalocation = msEncodeHTMLEntities(msOWSGetSchemasLocation(map));
        sprintf(szTmp, "<StyledLayerDescriptor version=\"1.0.0\" xmlns=\"http://www.opengis.net/sld\" xmlns:gml=\"http://www.opengis.net/gml\" xmlns:ogc=\"http://www.opengis.net/ogc\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.opengis.net/sld %s/sld/1.0.0/StyledLayerDescriptor.xsd\">\n",schemalocation );
        free(schemalocation);

        pszSLD = strcatalloc(pszSLD, szTmp);
        if (iLayer < 0 || iLayer > map->numlayers -1)
        {
            for (i=0; i<map->numlayers; i++)
            {
                pszTmp = msSLDGenerateSLDLayer(&map->layers[i]);
                if (pszTmp)
                {
                    pszSLD= strcatalloc(pszSLD, pszTmp);
                    free(pszTmp);
                }
            }
        }
        else
        {
             pszTmp = msSLDGenerateSLDLayer(&map->layers[iLayer]);
             if (pszTmp)
             {
                 pszSLD = strcatalloc(pszSLD, pszTmp);
                 free(pszTmp);
             }
        }
        sprintf(szTmp, "%s", "</StyledLayerDescriptor>\n");
        pszSLD = strcatalloc(pszSLD, szTmp);
    }

    return pszSLD;

#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

    msSetError(MS_MISCERR, "OGR support is not available.", "msSLDGenerateSLD()");
    return NULL;

#endif /* USE_OGR */

#else
    msSetError(MS_MISCERR, "OWS support is not available.", 
               "msSLDGenerateSLDLayer()");
    return NULL; 

#endif
}



/************************************************************************/
/*                            msSLDGetGraphicSLD                        */
/*                                                                      */
/*      Get an SLD for a sytle containg a symbol (Mark or external).    */
/************************************************************************/
char *msSLDGetGraphicSLD(styleObj *psStyle, layerObj *psLayer,
                         int bNeedMarkSybol)
{
    char *pszSLD = NULL;
    int nSymbol = -1;
    symbolObj *psSymbol = NULL;
    char szTmp[512];
    char *pszURL = NULL;
    char szFormat[4];
    int i = 0, nLength = 0;
    int bFillColor = 0, bColorAvailable=0;
    int bGenerateDefaultSymbol = 0;
    char *pszSymbolName= NULL;

    if (psStyle && psLayer && psLayer->map)
    {
        nSymbol = -1;
        if (psStyle->symbol > 0)
          nSymbol = psStyle->symbol;
        else if (psStyle->symbolname)
          nSymbol = msGetSymbolIndex(&psLayer->map->symbolset,
                                     psStyle->symbolname, MS_FALSE);

        bGenerateDefaultSymbol = 0;

        if (bNeedMarkSybol && 
            (nSymbol <=0 || nSymbol >=  psLayer->map->symbolset.numsymbols))
          bGenerateDefaultSymbol = 1;

        if (nSymbol > 0 && nSymbol < psLayer->map->symbolset.numsymbols)
        {
            psSymbol =  &psLayer->map->symbolset.symbol[nSymbol];
            if (psSymbol->type == MS_SYMBOL_VECTOR || 
                psSymbol->type == MS_SYMBOL_ELLIPSE)
            {
                /* Mark symbol */
                if (psSymbol->name)
                    
                {
                    if (strcasecmp(psSymbol->name, "square") == 0 ||
                        strcasecmp(psSymbol->name, "circle") == 0 ||
                        strcasecmp(psSymbol->name, "triangle") == 0 ||
                        strcasecmp(psSymbol->name, "star") == 0 ||
                        strcasecmp(psSymbol->name, "cross") == 0 ||
                        strcasecmp(psSymbol->name, "x") == 0)
                      pszSymbolName = strdup(psSymbol->name);
                    else if (strncasecmp(psSymbol->name, 
                                         "sld_mark_symbol_square", 22) == 0)
                      pszSymbolName = strdup("square");
                    else if (strncasecmp(psSymbol->name, 
                                         "sld_mark_symbol_triangle", 24) == 0)
                      pszSymbolName = strdup("triangle");
                    else if (strncasecmp(psSymbol->name, 
                                         "sld_mark_symbol_circle", 22) == 0)
                      pszSymbolName = strdup("circle");
                    else if (strncasecmp(psSymbol->name, 
                                         "sld_mark_symbol_star", 20) == 0)
                      pszSymbolName = strdup("star");
                    else if (strncasecmp(psSymbol->name, 
                                         "sld_mark_symbol_cross", 21) == 0)
                      pszSymbolName = strdup("cross");
                    else if (strncasecmp(psSymbol->name, 
                                         "sld_mark_symbol_x", 17) == 0)
                      pszSymbolName = strdup("X");
                             

                   
                    if (pszSymbolName)
                    {
                        colorObj sTmpColor;

                        sprintf(szTmp, "%s\n", "<Graphic>");
                        pszSLD = strcatalloc(pszSLD, szTmp);

                        

                        sprintf(szTmp, "%s\n", "<Mark>");
                        pszSLD = strcatalloc(pszSLD, szTmp);

                        sprintf(szTmp, "<WellKnownName>%s</WellKnownName>\n",
                                pszSymbolName);
                        pszSLD = strcatalloc(pszSLD, szTmp);

                        
                        if (psStyle->color.red != -1 && 
                            psStyle->color.green != -1 &&
                            psStyle->color.blue != -1)
                        {
                            sTmpColor.red = psStyle->color.red;
                            sTmpColor.green = psStyle->color.green;
                            sTmpColor.blue = psStyle->color.blue;
                            bFillColor =1;
                        }
                        else if (psStyle->outlinecolor.red != -1 && 
                                 psStyle->outlinecolor.green != -1 &&
                                 psStyle->outlinecolor.blue != -1) 
                        {
                            sTmpColor.red = psStyle->outlinecolor.red;
                            sTmpColor.green = psStyle->outlinecolor.green;
                            sTmpColor.blue = psStyle->outlinecolor.blue;
                            bFillColor = 0;
                        }
                        else
                        {
                            sTmpColor.red = 128;
                            sTmpColor.green = 128;
                            sTmpColor.blue = 128;
                             bFillColor =1;
                        }
                        
                        
                        if (psLayer->type == MS_LAYER_POINT)
                        {
                            if (psSymbol->filled)
                            {
                                sprintf(szTmp, "%s\n", "<Fill>");
                                pszSLD = strcatalloc(pszSLD, szTmp);
                                sprintf(szTmp, "<CssParameter name=\"fill\">#%02x%02x%02x</CssParameter>\n",
                                        sTmpColor.red,
                                        sTmpColor.green,
                                        sTmpColor.blue);
                            }
                            else
                            {
                                sprintf(szTmp, "%s\n", "<Stroke>");
                                pszSLD = strcatalloc(pszSLD, szTmp);
                                sprintf(szTmp, "<CssParameter name=\"stroke\">#%02x%02x%02x</CssParameter>\n",
                                        sTmpColor.red,
                                        sTmpColor.green,
                                        sTmpColor.blue);
                            }
                        }       
                        else    
                        {
                            if (bFillColor)
                            {
                                sprintf(szTmp, "%s\n", "<Fill>");
                                pszSLD = strcatalloc(pszSLD, szTmp);
                                sprintf(szTmp, "<CssParameter name=\"fill\">#%02x%02x%02x</CssParameter>\n",
                                        sTmpColor.red,
                                        sTmpColor.green,
                                        sTmpColor.blue);
                            }
                            else
                            {
                                sprintf(szTmp, "%s\n", "<Stroke>");
                                pszSLD = strcatalloc(pszSLD, szTmp);
                                sprintf(szTmp, "<CssParameter name=\"stroke\">#%02x%02x%02x</CssParameter>\n",
                                        sTmpColor.red,
                                        sTmpColor.green,
                                        sTmpColor.blue);
                            }
                        }
                        pszSLD = strcatalloc(pszSLD, szTmp);

                        if ((psLayer->type == MS_LAYER_POINT && psSymbol->filled) || 
                            bFillColor)
                           sprintf(szTmp, "%s\n", "</Fill>");
                        else
                          sprintf(szTmp, "%s\n", "</Stroke>");
                        pszSLD = strcatalloc(pszSLD, szTmp);

                        sprintf(szTmp, "%s\n", "</Mark>");
                        pszSLD = strcatalloc(pszSLD, szTmp);
                    
                        if (psStyle->size > 0)
                        {
                            sprintf(szTmp, "<Size>%d</Size>\n", psStyle->size);
                            pszSLD = strcatalloc(pszSLD, szTmp);
                        }

                        sprintf(szTmp, "%s\n", "</Graphic>");
                        pszSLD = strcatalloc(pszSLD, szTmp);

                        if (pszSymbolName)
                          free(pszSymbolName);
                    }
                }
                else
                  bGenerateDefaultSymbol =1;
            }
            else if (psSymbol->type == MS_SYMBOL_PIXMAP)
            {
                if (psSymbol->name)
                {
                    pszURL = msLookupHashTable(&(psLayer->metadata), "WMS_SLD_SYMBOL_URL");
                    if (!pszURL)
                      pszURL = msLookupHashTable(&(psLayer->map->web.metadata), "WMS_SLD_SYMBOL_URL");

                    if (pszURL)
                    {
                        sprintf(szTmp, "%s\n", "<Graphic>");
                        pszSLD = strcatalloc(pszSLD, szTmp);

                        

                        sprintf(szTmp, "%s\n", "<ExternalGraphic>");
                        pszSLD = strcatalloc(pszSLD, szTmp);
                        
                        sprintf(szTmp, "<OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:type=\"simple\" xlink:href=\"%s%s\"/>\n", 
                                pszURL,psSymbol->imagepath);
                        pszSLD = strcatalloc(pszSLD, szTmp);
                        /* TODO : extract format from symbol */

                        szFormat[0] = '\0';
                        nLength = strlen(psSymbol->imagepath);
                        if (nLength > 3)
                        {
                            for (i=0; i<=2; i++)
                              szFormat[2-i] = psSymbol->imagepath[nLength-1-i];
                            szFormat[3] = '\0';
                        }
                        if (strlen(szFormat) > 0 &&
                            ((strcasecmp (szFormat, "GIF") == 0) ||
                             (strcasecmp (szFormat, "PNG") == 0)))
                        {
                            if (strcasecmp (szFormat, "GIF") == 0)
                              sprintf(szTmp, "<Format>image/gif</Format>\n");
                            else
                              sprintf(szTmp, "<Format>image/png</Format>\n");
                        }
                        else
                          sprintf(szTmp, "<Format>%s</Format>\n", "image/gif");
                            
                        pszSLD = strcatalloc(pszSLD, szTmp);  

                        sprintf(szTmp, "%s\n",  "</ExternalGraphic>");
                        pszSLD = strcatalloc(pszSLD, szTmp);

                        if (psStyle->size > 0)
                          sprintf(szTmp, "<Size>%d</Size>\n", psStyle->size);
                        pszSLD = strcatalloc(pszSLD, szTmp);

                        sprintf(szTmp, "%s\n", "</Graphic>");
                        pszSLD = strcatalloc(pszSLD, szTmp);

                    }
                }
                    
            }
        }
        if (bGenerateDefaultSymbol) /* genrate a default square symbol */
        {
            sprintf(szTmp, "%s\n", "<Graphic>");
            pszSLD = strcatalloc(pszSLD, szTmp);

            

            sprintf(szTmp, "%s\n", "<Mark>");
            pszSLD = strcatalloc(pszSLD, szTmp);

            sprintf(szTmp, "<WellKnownName>%s</WellKnownName>\n",
                    "square");
            pszSLD = strcatalloc(pszSLD, szTmp);

            bColorAvailable = 0;
            if (psStyle->color.red != -1 && 
                psStyle->color.green != -1 &&
                psStyle->color.blue != -1)
            {
                sprintf(szTmp, "%s\n", "<Fill>");
                pszSLD = strcatalloc(pszSLD, szTmp);
                sprintf(szTmp, "<CssParameter name=\"fill\">#%02x%02x%02x</CssParameter>\n",
                        psStyle->color.red,
                        psStyle->color.green,
                        psStyle->color.blue);
                pszSLD = strcatalloc(pszSLD, szTmp);
                sprintf(szTmp, "%s\n", "</Fill>");
                pszSLD = strcatalloc(pszSLD, szTmp);
                bColorAvailable = 1;
            }
            if (psStyle->outlinecolor.red != -1 && 
                psStyle->outlinecolor.green != -1 &&
                psStyle->outlinecolor.blue != -1)    
            {
                sprintf(szTmp, "%s\n", "<Stroke>");
                pszSLD = strcatalloc(pszSLD, szTmp);
                sprintf(szTmp, "<CssParameter name=\"Stroke\">#%02x%02x%02x</CssParameter>\n",
                        psStyle->outlinecolor.red,
                        psStyle->outlinecolor.green,
                        psStyle->outlinecolor.blue);
                pszSLD = strcatalloc(pszSLD, szTmp);
                sprintf(szTmp, "%s\n", "</Stroke>");
                pszSLD = strcatalloc(pszSLD, szTmp);
                bColorAvailable = 1;
            }
            if (!bColorAvailable)
            {       
                /* default color */
                sprintf(szTmp, 
                        "<CssParameter name=\"fill\">%s</CssParameter>\n",
                        "#808080");
                pszSLD = strcatalloc(pszSLD, szTmp);
                sprintf(szTmp, "%s\n", "</Fill>");
                pszSLD = strcatalloc(pszSLD, szTmp);
            }

            sprintf(szTmp, "%s\n", "</Mark>");
            pszSLD = strcatalloc(pszSLD, szTmp);

            if (psStyle->size > 0)
              sprintf(szTmp, "<Size>%d</Size>\n", psStyle->size);
            else
              sprintf(szTmp, "<Size>%d</Size>\n", 1);
            pszSLD = strcatalloc(pszSLD, szTmp);

            sprintf(szTmp, "%s\n", "</Graphic>");
            pszSLD = strcatalloc(pszSLD, szTmp);
            
        
        }
    }

    return pszSLD;
}




/************************************************************************/
/*                           msSLDGenerateLineSLD                       */
/*                                                                      */
/*      Generate SLD for a Line layer.                                  */
/************************************************************************/
char *msSLDGenerateLineSLD(styleObj *psStyle, layerObj *psLayer)
{
    char *pszSLD = NULL;
    char szTmp[100];
    char szHexColor[7];
    int nSymbol = -1;
    symbolObj *psSymbol = NULL;
    int i = 0;
    int nSize = 1;
    char *pszDashArray = NULL;
    char *pszGraphicSLD = NULL;

    sprintf(szTmp, "%s\n",  "<LineSymbolizer>");
    pszSLD = strcatalloc(pszSLD, szTmp);

    sprintf(szTmp, "%s\n",  "<Stroke>");
    pszSLD = strcatalloc(pszSLD, szTmp);

    /* TODO : does not work (color is not picked) */
    /* pszGraphicSLD = msSLDGetGraphicSLD(psStyle, psLayer); */
    if (pszGraphicSLD)
    {
        sprintf(szTmp, "%s\n",  "<GraphicFill>");
        pszSLD = strcatalloc(pszSLD, szTmp);
        
        pszSLD = strcatalloc(pszSLD, pszGraphicSLD);
        sprintf(szTmp, "%s\n",  "</GraphicFill>");
        pszSLD = strcatalloc(pszSLD, szTmp);
             
        free(pszGraphicSLD);
        pszGraphicSLD = NULL;
    }

    sprintf(szHexColor,"%02x%02x%02x",psStyle->color.red,
            psStyle->color.green,psStyle->color.blue);
                            
    sprintf(szTmp, 
            "<CssParameter name=\"stroke\">#%s</CssParameter>\n", 
            szHexColor);
    pszSLD = strcatalloc(pszSLD, szTmp);
                            
    nSymbol = -1;

    if (psStyle->symbol > 0)
      nSymbol = psStyle->symbol;
    else if (psStyle->symbolname)
      nSymbol = msGetSymbolIndex(&psLayer->map->symbolset,
                                 psStyle->symbolname, MS_FALSE);
                            
    /* if no symbol or symbol 0 is used, size is set to 1 */
    /* which is the way mapserver works */
    if (nSymbol <=0)
      nSize = 1;
    else
      nSize = psStyle->size;

    sprintf(szTmp, 
            "<CssParameter name=\"stroke-width\">%d</CssParameter>\n",
            nSize);
    pszSLD = strcatalloc(pszSLD, szTmp);
                            
/* -------------------------------------------------------------------- */
/*      dash array                                                      */
/* -------------------------------------------------------------------- */
                            
                            
    if (nSymbol > 0 && nSymbol < psLayer->map->symbolset.numsymbols)
    {
        psSymbol =  &psLayer->map->symbolset.symbol[nSymbol];
        if (psSymbol->stylelength > 0)
        {
            for (i=0; i<psSymbol->stylelength; i++)
            {
                sprintf(szTmp, "%d ", psSymbol->style[i]);
                pszDashArray = strcatalloc(pszDashArray, szTmp);
            }
            sprintf(szTmp, 
                    "<CssParameter name=\"stroke-dasharray\">%s</CssParameter>\n", 
                    pszDashArray);
            pszSLD = strcatalloc(pszSLD, szTmp);
        }  
                                           
    }

    
    sprintf(szTmp, "%s\n",  "</Stroke>");
    pszSLD = strcatalloc(pszSLD, szTmp);

    sprintf(szTmp, "%s\n",  "</LineSymbolizer>");
    pszSLD = strcatalloc(pszSLD, szTmp);

    return pszSLD;

}


/************************************************************************/
/*                         msSLDGeneratePolygonSLD                      */
/*                                                                      */
/*       Generate SLD for a Polygon layer.                              */
/************************************************************************/
char *msSLDGeneratePolygonSLD(styleObj *psStyle, layerObj *psLayer)
{
    char szTmp[100];
    char *pszSLD = NULL;
    char szHexColor[7];
    char *pszGraphicSLD = NULL;

    sprintf(szTmp, "%s\n",  "<PolygonSymbolizer>");
    pszSLD = strcatalloc(pszSLD, szTmp);
    /* fill */
    if (psStyle->color.red != -1 && psStyle->color.green != -1 &&
        psStyle->color.blue != -1)
    {

        sprintf(szTmp, "%s\n",  "<Fill>");
        pszSLD = strcatalloc(pszSLD, szTmp);
        
        

        pszGraphicSLD = msSLDGetGraphicSLD(psStyle, psLayer, 0);
        if (pszGraphicSLD)
        {
             sprintf(szTmp, "%s\n",  "<GraphicFill>");
             pszSLD = strcatalloc(pszSLD, szTmp);

             pszSLD = strcatalloc(pszSLD, pszGraphicSLD);
             sprintf(szTmp, "%s\n",  "</GraphicFill>");
             pszSLD = strcatalloc(pszSLD, szTmp);
             
             free(pszGraphicSLD);
             pszGraphicSLD = NULL;
        }

        sprintf(szHexColor,"%02x%02x%02x",psStyle->color.red,
                psStyle->color.green,psStyle->color.blue);
                             
        sprintf(szTmp, 
                "<CssParameter name=\"fill\">#%s</CssParameter>\n", 
                szHexColor);
        pszSLD = strcatalloc(pszSLD, szTmp);

        sprintf(szTmp, "%s\n",  "</Fill>");
        pszSLD = strcatalloc(pszSLD, szTmp);
    }
    /* stroke */
    if (psStyle->outlinecolor.red != -1 && 
        psStyle->outlinecolor.green != -1 &&
        psStyle->outlinecolor.blue != -1)
    {
        sprintf(szTmp, "%s\n",  "<Stroke>");
        pszSLD = strcatalloc(pszSLD, szTmp);

        

        /* If there is a symbol to be used for sroke, the color in the */
        /* style sholud be set to -1. Else It won't apply here. */
        if (psStyle->color.red == -1 && psStyle->color.green == -1 &&
            psStyle->color.blue == -1)
        {
            pszGraphicSLD = msSLDGetGraphicSLD(psStyle, psLayer, 0);
            if (pszGraphicSLD)
            {
                sprintf(szTmp, "%s\n",  "<GraphicFill>");
                pszSLD = strcatalloc(pszSLD, szTmp);

                pszSLD = strcatalloc(pszSLD, pszGraphicSLD);
                sprintf(szTmp, "%s\n",  "</GraphicFill>");
                pszSLD = strcatalloc(pszSLD, szTmp);
             
                free(pszGraphicSLD);
                pszGraphicSLD = NULL;
            }
        }

        sprintf(szHexColor,"%02x%02x%02x",psStyle->outlinecolor.red,
                psStyle->outlinecolor.green,
                psStyle->outlinecolor.blue);
        
        sprintf(szTmp, 
                "<CssParameter name=\"stroke\">#%s</CssParameter>\n", 
                szHexColor);
        pszSLD = strcatalloc(pszSLD, szTmp);

        sprintf(szTmp, "%s\n",  "</Stroke>");
        pszSLD = strcatalloc(pszSLD, szTmp);
    }

    sprintf(szTmp, "%s\n",  "</PolygonSymbolizer>");
    pszSLD = strcatalloc(pszSLD, szTmp);

    return pszSLD;
}

/************************************************************************/
/*                          msSLDGeneratePointSLD                       */
/*                                                                      */
/*      Generate SLD for a Point layer.                                 */
/************************************************************************/
char *msSLDGeneratePointSLD(styleObj *psStyle, layerObj *psLayer)
{
    char *pszSLD = NULL;
    char *pszGraphicSLD = NULL;
    char szTmp[100];

    sprintf(szTmp, "%s\n",  "<PointSymbolizer>");
    pszSLD = strcatalloc(pszSLD, szTmp);

    pszGraphicSLD = msSLDGetGraphicSLD(psStyle, psLayer, 1);
    if (pszGraphicSLD)
    {
        pszSLD = strcatalloc(pszSLD, pszGraphicSLD);
        free(pszGraphicSLD);
    }
    
    sprintf(szTmp, "%s\n",  "</PointSymbolizer>");
    pszSLD = strcatalloc(pszSLD, szTmp);

    return pszSLD;
}



/************************************************************************/
/*                           msSLDGenerateTextSLD                       */
/*                                                                      */
/*      Generate a TextSymboliser SLD xml based on the class's label    */
/*      object.                                                         */
/************************************************************************/
char *msSLDGenerateTextSLD(classObj *psClass, layerObj *psLayer)
{
    char *pszSLD = NULL;

    char szTmp[100];
    char **aszFontsParts = NULL;
    int nFontParts = 0;
    char szHexColor[7];
    int nColorRed=-1, nColorGreen=-1, nColorBlue=-1;
    double dfAnchorX = 0.5, dfAnchorY = 0.5;
    int i = 0;

    if (psClass && psLayer && psLayer->labelitem && 
        strlen(psLayer->labelitem) > 0)
    {
        sprintf(szTmp, "%s\n",  "<TextSymbolizer>");
        pszSLD = strcatalloc(pszSLD, szTmp);

        sprintf(szTmp, "<Label>%s</Label>\n",  psLayer->labelitem);
        pszSLD = strcatalloc(pszSLD, szTmp);

/* -------------------------------------------------------------------- */
/*      only true type fonta are exported. Font name should be          */
/*      something like arial-bold-italic. There are 3 parts to the      */
/*      name font-family, font-style (italic, oblique, nomal),          */
/*      font-weight (bold, normal). These 3 elements are separated      */
/*      with -.                                                         */
/* -------------------------------------------------------------------- */
        if (psClass->label.type == MS_TRUETYPE && psClass->label.font)
        {
            aszFontsParts = split(psClass->label.font, '-', &nFontParts);
            if (nFontParts > 0)
            {
                sprintf(szTmp, "%s\n",  "<Font>");
                pszSLD = strcatalloc(pszSLD, szTmp);

                /* assuming first one is font-family */
                sprintf(szTmp, 
                        "<CssParameter name=\"font-family\">%s</CssParameter>\n",
                        aszFontsParts[0]);
                pszSLD = strcatalloc(pszSLD, szTmp);
                for (i=1; i<nFontParts; i++)
                {
                    if (strcasecmp(aszFontsParts[i], "italic") == 0 ||
                        strcasecmp(aszFontsParts[i], "oblique") == 0)
                    {
                        sprintf(szTmp, 
                        "<CssParameter name=\"font-style\">%s</CssParameter>\n",
                        aszFontsParts[i]);
                        pszSLD = strcatalloc(pszSLD, szTmp);
                    }
                    else if (strcasecmp(aszFontsParts[i], "bold") == 0)
                    {
                        sprintf(szTmp, 
                        "<CssParameter name=\"font-weight\">%s</CssParameter>\n",
                        aszFontsParts[i]);
                        pszSLD = strcatalloc(pszSLD, szTmp);
                    }
                }
                /* size */
                if (psClass->label.size > 0)
                {
                    sprintf(szTmp, 
                            "<CssParameter name=\"font-size\">%d</CssParameter>\n",
                            psClass->label.size);
                    pszSLD = strcatalloc(pszSLD, szTmp);
                }
                sprintf(szTmp, "%s\n",  "</Font>");
                pszSLD = strcatalloc(pszSLD, szTmp);

                msFreeCharArray(aszFontsParts, nFontParts);
            }
        }
        
        
        /* label placement */
        sprintf(szTmp, "%s\n%s\n",  "<LabelPlacement>", "<PointPlacement>");
        pszSLD = strcatalloc(pszSLD, szTmp);

        sprintf(szTmp, "%s\n",  "<AnchorPoint>");
        pszSLD = strcatalloc(pszSLD, szTmp);

        if (psClass->label.position == MS_LL)
        {
            dfAnchorX =0; 
            dfAnchorY = 0;
        }
        else if (psClass->label.position == MS_CL)
        {
            dfAnchorX =0; 
            dfAnchorY = 0.5;
        }
        else if (psClass->label.position == MS_UL)
        {
            dfAnchorX =0; 
            dfAnchorY = 1;
        }
        
        else if (psClass->label.position == MS_LC)
        {
            dfAnchorX =0.5; 
            dfAnchorY = 0;
        }
        else if (psClass->label.position == MS_CC)
        {
            dfAnchorX =0.5; 
            dfAnchorY = 0.5;
        }
        else if (psClass->label.position == MS_UC)
        {
            dfAnchorX =0.5; 
            dfAnchorY = 1;
        }
        
        else if (psClass->label.position == MS_LR)
        {
            dfAnchorX =1; 
            dfAnchorY = 0;
        }
        else if (psClass->label.position == MS_CR)
        {
            dfAnchorX =1; 
            dfAnchorY = 0.5;
        }
        else if (psClass->label.position == MS_UR)
        {
            dfAnchorX =1; 
            dfAnchorY = 1;
        }
        sprintf(szTmp, "<AnchorPointX>%.1f</AnchorPointX>\n", dfAnchorX);
        pszSLD = strcatalloc(pszSLD, szTmp);
        sprintf(szTmp, "<AnchorPointY>%.1f</AnchorPointY>\n", dfAnchorY);
        pszSLD = strcatalloc(pszSLD, szTmp);

        sprintf(szTmp, "%s\n",  "</AnchorPoint>");
        pszSLD = strcatalloc(pszSLD, szTmp);

        /* displacement */
        if (psClass->label.offsetx > 0 || psClass->label.offsety > 0)
        {
            sprintf(szTmp, "%s\n",  "<Displacement>");
            pszSLD = strcatalloc(pszSLD, szTmp);

            if (psClass->label.offsetx > 0)
            {
                sprintf(szTmp, "<DisplacementX>%d</DisplacementX>\n", 
                        psClass->label.offsetx);
                pszSLD = strcatalloc(pszSLD, szTmp);
            }
            if (psClass->label.offsety > 0)
            {
                sprintf(szTmp, "<DisplacementY>%d</DisplacementY>\n", 
                        psClass->label.offsety);
                pszSLD = strcatalloc(pszSLD, szTmp);
            }

            sprintf(szTmp, "%s\n",  "</Displacement>");
            pszSLD = strcatalloc(pszSLD, szTmp);
        }
        /* rotation */
        if (psClass->label.angle > 0)
        {
            sprintf(szTmp, "<Rotation>%.2f</Rotation>\n", 
                        psClass->label.angle);
            pszSLD = strcatalloc(pszSLD, szTmp);
        }

         /* TODO : support Halo parameter => shadow */

        sprintf(szTmp, "%s\n%s\n",  "</PointPlacement>", "</LabelPlacement>");
        pszSLD = strcatalloc(pszSLD, szTmp);


        /* color */
        if (psClass->label.color.red != -1 &&
            psClass->label.color.green != -1 &&
            psClass->label.color.blue != -1)
        {
            nColorRed = psClass->label.color.red;
            nColorGreen = psClass->label.color.green;
            nColorBlue = psClass->label.color.blue;
        }
        else if (psClass->label.outlinecolor.red != -1 &&
                 psClass->label.outlinecolor.green != -1 &&
                 psClass->label.outlinecolor.blue != -1)
        {
            nColorRed = psClass->label.outlinecolor.red;
            nColorGreen = psClass->label.outlinecolor.green;
            nColorBlue = psClass->label.outlinecolor.blue;
        }
        if (nColorRed >= 0 && nColorGreen >= 0  && nColorBlue >=0)
        {
            sprintf(szTmp, "%s\n",  "<Fill>");
            pszSLD = strcatalloc(pszSLD, szTmp);
            
            sprintf(szHexColor,"%02x%02x%02x",nColorRed,
                    nColorGreen, nColorBlue);

            sprintf(szTmp, 
                    "<CssParameter name=\"fill\">#%s</CssParameter>\n", 
                    szHexColor);
            pszSLD = strcatalloc(pszSLD, szTmp);

            sprintf(szTmp, "%s\n",  "</Fill>");
            pszSLD = strcatalloc(pszSLD, szTmp);
        }
        
        sprintf(szTmp, "%s\n",  "</TextSymbolizer>");
        pszSLD = strcatalloc(pszSLD, szTmp);
    }
    return pszSLD;

}


/************************************************************************/
/*                          msSLDGenerateSLDLayer                       */
/*                                                                      */
/*      Genrate an SLD XML string based on the layer's classes.         */
/************************************************************************/
char *msSLDGenerateSLDLayer(layerObj *psLayer)
{
#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR) || defined(USE_SOS_SVR)

#ifdef USE_OGR
    char szTmp[100];
    int i, j;
    styleObj *psStyle = NULL;
    char *pszFilter = NULL;
    char *pszFinalSLD = NULL;
    char *pszSLD = NULL;
    const char *pszTmp = NULL;
    double dfMinScale =-1, dfMaxScale = -1;
    const char *pszWfsFilter= NULL;
    char *pszEncoded = NULL, *pszWfsFilterEncoded=NULL;

    
    if (psLayer && 
        (psLayer->status == MS_ON || psLayer->status == MS_DEFAULT) &&
        (psLayer->type == MS_LAYER_POINT ||
                    psLayer->type == MS_LAYER_LINE ||
                    psLayer->type == MS_LAYER_POLYGON ||
                    psLayer->type == MS_LAYER_ANNOTATION))
    {
        sprintf(szTmp, "%s\n",  "<NamedLayer>");
        pszFinalSLD = strcatalloc(pszFinalSLD, szTmp);

        pszTmp = msOWSLookupMetadata(&(psLayer->metadata), "MO", "name");
        if (pszTmp)
        {
            pszEncoded = msEncodeHTMLEntities(pszTmp);
            sprintf(szTmp, "<Name>%s</Name>\n", pszEncoded);
            msFree(pszEncoded);
        }
        else if (psLayer->name)
        {
            pszEncoded = msEncodeHTMLEntities(psLayer->name);
            sprintf(szTmp, "<Name>%s</Name>\n", pszEncoded);
            msFree(pszEncoded);
        }
        else
          sprintf(szTmp, "<Name>%s</Name>\n", "NamedLayer");

        pszFinalSLD = strcatalloc(pszFinalSLD, szTmp);

        sprintf(szTmp, "%s\n",  "<UserStyle>");
        pszFinalSLD = strcatalloc(pszFinalSLD, szTmp);

        sprintf(szTmp, "%s\n",  "<FeatureTypeStyle>");
        pszFinalSLD = strcatalloc(pszFinalSLD, szTmp);

        pszWfsFilter = msLookupHashTable(&(psLayer->metadata), "wfs_filter");
        if (pszWfsFilter)
          pszWfsFilterEncoded = msEncodeHTMLEntities(pszWfsFilter);
        if (psLayer->numclasses > 0)
        {
            for (i=psLayer->numclasses-1; i>=0; i--)
            {
                sprintf(szTmp, "%s\n",  "<Rule>");
                pszFinalSLD = strcatalloc(pszFinalSLD, szTmp);

                /* if class has a name, use it as the RULE name */
                if (psLayer->class[i].name)
                {
                    pszEncoded = msEncodeHTMLEntities(psLayer->class[i].name);
                    sprintf(szTmp, "<Name>%s</Name>\n",  pszEncoded);
                    msFree(pszEncoded);

                    pszFinalSLD = strcatalloc(pszFinalSLD, szTmp);
                }
/* -------------------------------------------------------------------- */
/*      get the Filter if there is a class expression.                  */
/* -------------------------------------------------------------------- */
                pszFilter = msSLDGetFilter(&psLayer->class[i], 
                                           pszWfsFilter);/* pszWfsFilterEncoded); */
                    
                if (pszFilter)
                {
                    pszFinalSLD = strcatalloc(pszFinalSLD, pszFilter);
                    free(pszFilter);
                }
/* -------------------------------------------------------------------- */
/*      generate the min/max scale.                                     */
/* -------------------------------------------------------------------- */
                dfMinScale = -1.0;
                if (psLayer->class[i].minscale > 0)
                  dfMinScale = psLayer->class[i].minscale;      
                else if (psLayer->minscale > 0)
                  dfMinScale = psLayer->minscale;
                else if (psLayer->map && psLayer->map->web.minscale > 0)
                  dfMinScale = psLayer->map->web.minscale;
                if (dfMinScale > 0)
                {
                     sprintf(szTmp, "<MinScaleDenominator>%f</MinScaleDenominator>\n",  
                             dfMinScale);
                     pszFinalSLD = strcatalloc(pszFinalSLD, szTmp);
                }
               
                dfMaxScale = -1.0;
                if (psLayer->class[i].maxscale > 0)
                  dfMaxScale = psLayer->class[i].maxscale;      
                else if (psLayer->maxscale > 0)
                  dfMaxScale = psLayer->maxscale;
                else if (psLayer->map && psLayer->map->web.maxscale > 0)
                  dfMaxScale = psLayer->map->web.maxscale;
                if (dfMaxScale > 0)
                {
                     sprintf(szTmp, "<MaxScaleDenominator>%f</MaxScaleDenominator>\n",  
                             dfMaxScale);
                     pszFinalSLD = strcatalloc(pszFinalSLD, szTmp);
                }
                 

                  
/* -------------------------------------------------------------------- */
/*      Line symbolizer.                                                */
/*                                                                      */
/*      Right now only generates a stroke element containing css        */
/*      parameters.                                                     */
/*      Lines using symbols TODO (specially for dash lines)             */
/* -------------------------------------------------------------------- */
                if (psLayer->type == MS_LAYER_LINE)
                {
                    for (j=0; j<psLayer->class[i].numstyles; j++)
                    {
                        psStyle = &psLayer->class[i].styles[j];
                        pszSLD = msSLDGenerateLineSLD(psStyle, psLayer);
                        if (pszSLD)
                        {
                            pszFinalSLD = strcatalloc(pszFinalSLD, pszSLD);
                            free(pszSLD);
                        }
                    }
                   
                }
                else if (psLayer->type == MS_LAYER_POLYGON)
                {
                    for (j=0; j<psLayer->class[i].numstyles; j++)
                    {
                        psStyle = &psLayer->class[i].styles[j];
                        pszSLD = msSLDGeneratePolygonSLD(psStyle, psLayer);
                        if (pszSLD)
                        {
                            pszFinalSLD = strcatalloc(pszFinalSLD, pszSLD);
                            free(pszSLD);
                        }
                    } 
                    
                }
                else if (psLayer->type == MS_LAYER_POINT)
                {
                    for (j=0; j<psLayer->class[i].numstyles; j++)
                    {
                        psStyle = &psLayer->class[i].styles[j];
                        pszSLD = msSLDGeneratePointSLD(psStyle, psLayer);
                        if (pszSLD)
                        {
                            pszFinalSLD = strcatalloc(pszFinalSLD, pszSLD);
                            free(pszSLD);
                        }
                    } 
                    
                }
                else if (psLayer->type == MS_LAYER_ANNOTATION)
                {
                    for (j=0; j<psLayer->class[i].numstyles; j++)
                    {
                        psStyle = &psLayer->class[i].styles[j];
                        pszSLD = msSLDGeneratePointSLD(psStyle, psLayer);
                        if (pszSLD)
                        {
                            pszFinalSLD = strcatalloc(pszFinalSLD, pszSLD);
                            free(pszSLD);
                        }
                    } 
                    
                }
                /* label if it exists */
                pszSLD = msSLDGenerateTextSLD(&psLayer->class[i], psLayer);
                if (pszSLD)
                {
                    pszFinalSLD = strcatalloc(pszFinalSLD, pszSLD);
                    free(pszSLD);
                }
                sprintf(szTmp, "%s\n",  "</Rule>");
                pszFinalSLD = strcatalloc(pszFinalSLD, szTmp);
                
            
            }
        }
        if (pszWfsFilterEncoded)
          msFree(pszWfsFilterEncoded);
        sprintf(szTmp, "%s\n",  "</FeatureTypeStyle>");
        pszFinalSLD = strcatalloc(pszFinalSLD, szTmp);

        sprintf(szTmp, "%s\n",  "</UserStyle>");
        pszFinalSLD = strcatalloc(pszFinalSLD, szTmp);

        sprintf(szTmp, "%s\n",  "</NamedLayer>");
        pszFinalSLD = strcatalloc(pszFinalSLD, szTmp);
        
    }
    return pszFinalSLD;


#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

    msSetError(MS_MISCERR, "OGR support is not available.", 
               "msSLDGenerateSLDLayer()");
    return NULL;

#endif /* USE_OGR */


#else
    msSetError(MS_MISCERR, "OWS support is not available.", 
               "msSLDGenerateSLDLayer()");
    return NULL; 
#endif
}

#ifdef USE_OGR





char *msSLDGetComparisonValue(char *pszExpression)
{
    char *pszValue = NULL;
    if (!pszExpression)
      return NULL;

    if (strstr(pszExpression, "<=") || strstr(pszExpression, " le "))
      pszValue = strdup("PropertyIsLessThanOrEqualTo");
    else if (strstr(pszExpression, ">=") || strstr(pszExpression, " ge "))
      pszValue = strdup("PropertyIsGreaterThanOrEqualTo");
    else if (strstr(pszExpression, "!=") || strstr(pszExpression, " ne "))
      pszValue = strdup("PropertyIsNotEqualTo");
    else if (strstr(pszExpression, "=") || strstr(pszExpression, " eq "))
      pszValue = strdup("PropertyIsEqualTo");
    else if (strstr(pszExpression, "<") || strstr(pszExpression, " lt "))
      pszValue = strdup("PropertyIsLessThan");
    else if (strstr(pszExpression, ">") || strstr(pszExpression, " gt "))
      pszValue = strdup("PropertyIsGreaterThan");

    return pszValue;
}

char *msSLDGetLogicalOperator(char *pszExpression)
{

    if (!pszExpression)
      return NULL;

    /* TODO for NOT */

    if(strstr(pszExpression, " AND ") || strstr(pszExpression, "AND("))
      return strdup("And");
    
    if(strstr(pszExpression, " OR ") || strstr(pszExpression, "OR("))
      return strdup("Or");

     if(strstr(pszExpression, "NOT ") || strstr(pszExpression, "NOT("))
      return strdup("Not");

    return NULL;
}

char *msSLDGetRightExpressionOfOperator(char *pszExpression)
{
    char *pszAnd = NULL, *pszOr = NULL, *pszNot=NULL;

    pszAnd = strstr(pszExpression, " AND "); 
    if (!pszAnd)
      strstr(pszExpression, " and ");
  
    if (pszAnd)
      return strdup(pszAnd+4);
    else
    {
        pszOr = strstr(pszExpression, " OR "); 
        if (!pszOr)
          strstr(pszExpression, " or ");

        if (pszOr)
          return strdup(pszOr+3);
        else
        {
            pszNot = strstr(pszExpression, "NOT ");
            if (!pszNot)
              pszNot = strstr(pszExpression, "not ");
            if (!pszNot)
              strstr(pszExpression, "NOT(");
            if (!pszNot)
              strstr(pszExpression, "not(");

            if (pszNot)
              return strdup(pszNot+4);
        }
    }
    return NULL;
        
}

char *msSLDGetLeftExpressionOfOperator(char *pszExpression)
{
    char *pszReturn = NULL;
    int nLength = 0, i =0, iReturn=0;

    if (!pszExpression || (nLength = strlen(pszExpression)) <=0)
      return NULL;

    pszReturn = (char *)malloc(sizeof(char)*(nLength+1));
    pszReturn[0] = '\0';
    if (strstr(pszExpression, " AND ") || strstr(pszExpression, " and "))
    {
        for (i=0; i<nLength-5; i++)
        {
            if (pszExpression[i] == ' ' && 
                (pszExpression[i+1] == 'A' || pszExpression[i] == 'a') &&
                (pszExpression[i+2] == 'N' || pszExpression[i] == 'n') &&
                (pszExpression[i+3] == 'D' || pszExpression[i] == 'd') &&
                (pszExpression[i+4] == ' '))
              break;
            else
            {
                pszReturn[iReturn++] = pszExpression[i];
            }
            pszReturn[iReturn] = '\0';
        }
    }
    else if (strstr(pszExpression, "AND(") || strstr(pszExpression, "and("))
    {
        for (i=0; i<nLength-4; i++)
        {
            if ((pszExpression[i] == 'A' || pszExpression[i] == 'a') &&
                (pszExpression[i+1] == 'N' || pszExpression[i] == 'n') &&
                (pszExpression[i+2] == 'D' || pszExpression[i] == 'd') &&
                (pszExpression[i+3] == '('))
              break;
            else
            {
                pszReturn[iReturn++] = pszExpression[i];
            }
            pszReturn[iReturn] = '\0';
        }
    }
    else if (strstr(pszExpression, " OR ") || strstr(pszExpression, " or "))
    {
        for (i=0; i<nLength-4; i++)
        {
            if (pszExpression[i] == ' ' && 
                (pszExpression[i+1] == 'O' || pszExpression[i] == 'o') &&
                (pszExpression[i+2] == 'R' || pszExpression[i] == 'r') &&
                pszExpression[i+3] == ' ')
              break;
            else
            {
                pszReturn[iReturn++] = pszExpression[i];
            }
            pszReturn[iReturn] = '\0';
        }
    }
    else if (strstr(pszExpression, "OR(") || strstr(pszExpression, " or("))
    {
        for (i=0; i<nLength-3; i++)
        {
            if ((pszExpression[i] == 'O' || pszExpression[i] == 'o') &&
                (pszExpression[i+1] == 'R' || pszExpression[i] == 'r') &&
                pszExpression[i+2] == '(')
              break;
            else
            {
                pszReturn[iReturn++] = pszExpression[i];
            }
            pszReturn[iReturn] = '\0';
        }
    }
    else
      return NULL;

    return pszReturn;
}


int msSLDNumberOfLogicalOperators(char *pszExpression)
{
    char *pszAnd = NULL;
    char *pszOr = NULL;
    char *pszNot = NULL;
    char *pszSecondAnd=NULL, *pszSecondOr=NULL;
    if (!pszExpression)
      return 0;

/* -------------------------------------------------------------------- */
/*      tests here are minimal to be able to parse simple expression    */
/*      like A AND B, A OR B, NOT A.                                    */
/* -------------------------------------------------------------------- */
    pszAnd = strstr(pszExpression, " AND ");
    if (!pszAnd)
       pszAnd = strstr(pszExpression, " and ");

    pszOr = strstr(pszExpression, " OR ");
    if (!pszOr)
      pszOr = strstr(pszExpression, " or ");
    

    pszNot = strstr(pszExpression, "NOT ");
    if (!pszNot)
    {
        pszNot = strstr(pszExpression, "not ");
    }
    

    if (!pszAnd && !pszOr)
    {
        pszAnd = strstr(pszExpression, "AND(");
        if (!pszAnd)
          pszAnd = strstr(pszExpression, "and(");
        
        pszOr = strstr(pszExpression, "OR(");
        if (!pszOr)
          strstr(pszExpression, "or(");
    }

    if (!pszAnd && !pszOr && !pszNot)
      return 0;

    /* doen not matter how many exactly if there are 2 or more */
    if ((pszAnd && pszOr) || (pszAnd && pszNot) || (pszOr && pszNot)) 
      return 2;

    if (pszAnd)
    {
        pszSecondAnd = strstr(pszAnd+3, " AND ");
        if (!pszSecondAnd)
           pszSecondAnd = strstr(pszAnd+3, " and ");

        pszSecondOr = strstr(pszAnd+3, " OR ");
        if (!pszSecondOr)
          strstr(pszAnd+3, " or ");
    }
    else if (pszOr)
    {
        pszSecondAnd = strstr(pszOr+2, " AND ");
        if (!pszSecondAnd)
          pszSecondAnd = strstr(pszOr+2, " and ");

        pszSecondOr = strstr(pszOr+2, " OR ");
        if (!pszSecondOr)
          pszSecondOr = strstr(pszOr+2, " or ");

    }

    if (!pszSecondAnd && !pszSecondOr)
      return 1;
    else
      return 2;
}


char *msSLDGetAttributeNameOrValue(char *pszExpression, 
                                   char *pszComparionValue,
                                   int bReturnName)
{
    char **aszValues = NULL;
    char *pszAttributeName = NULL;
    char *pszAttributeValue = NULL;
    char cCompare = '=';
    char szCompare[3] = {0};
    char szCompare2[3] = {0};
    int bOneCharCompare = -1, nTokens = 0, nLength =0;
    int iValue=0, i=0, iValueIndex =0;
    int bStartCopy=0, iAtt=0;
    char *pszFinalAttributeName=NULL, *pszFinalAttributeValue=NULL;
    int bSingleQuote = 0, bDoubleQuote = 0;

    if (!pszExpression || !pszComparionValue || strlen(pszExpression) <=0)
      return NULL;

    szCompare[0] = '\0';
    szCompare2[0] = '\0';
    

    if (strcasecmp(pszComparionValue, "PropertyIsEqualTo") == 0)
    {
        cCompare = '=';
        szCompare[0] = 'e';
        szCompare[1] = 'q';
        szCompare[2] = '\0';

        bOneCharCompare =1;
    }
    if (strcasecmp(pszComparionValue, "PropertyIsNotEqualTo") == 0)
    {
        szCompare[0] = 'n';
        szCompare[1] = 'e';
        szCompare[2] = '\0';

        szCompare2[0] = '!';
        szCompare2[1] = '=';
        szCompare2[2] = '\0';

        bOneCharCompare =0;
    }
    else if (strcasecmp(pszComparionValue, "PropertyIsLessThan") == 0)
    {
        cCompare = '<';
        szCompare[0] = 'l';
        szCompare[1] = 't';
        szCompare[2] = '\0';
        bOneCharCompare =1;
    }
    else if (strcasecmp(pszComparionValue, "PropertyIsLessThanOrEqualTo") == 0)
    {
        szCompare[0] = 'l';
        szCompare[1] = 'e';
        szCompare[2] = '\0';

        szCompare2[0] = '<';
        szCompare2[1] = '=';
        szCompare2[2] = '\0';

        bOneCharCompare =0;
    }
    else if (strcasecmp(pszComparionValue, "PropertyIsGreaterThan") == 0)
    {
        cCompare = '>';
        szCompare[0] = 'g';
        szCompare[1] = 't';
        szCompare[2] = '\0';
        bOneCharCompare =1;
    }
    else if (strcasecmp(pszComparionValue, "PropertyIsGreaterThanOrEqualTo") == 0)
    {
        szCompare[0] = 'g';
        szCompare[1] = 'e';
        szCompare[2] = '\0';

        szCompare2[0] = '>';
        szCompare2[1] = '=';
        szCompare2[2] = '\0';

        bOneCharCompare =0;
    }

    if (bOneCharCompare == 1)
    {
        aszValues= split (pszExpression, cCompare, &nTokens);
        if (nTokens >= 1)
        {
            pszAttributeName = strdup(aszValues[0]);
            pszAttributeValue =  strdup(aszValues[1]);
            msFreeCharArray(aszValues, nTokens);
        }
        else
        {
            nLength = strlen(pszExpression);
            pszAttributeName = (char *)malloc(sizeof(char)*(nLength+1));
            iValue = 0;
            for (i=0; i<nLength-2; i++)
            {
                if (pszExpression[i] != szCompare[0] || 
                    pszExpression[i] != toupper(szCompare[0]))
                {
                    pszAttributeName[iValue++] = pszExpression[i];
                }
                else
                {
                    if ((pszExpression[i+1] == szCompare[1] || 
                         pszExpression[i+1] == toupper(szCompare[1])) &&
                        (pszExpression[i+2] == ' '))
                    {
                        iValueIndex = i+3;
                        pszAttributeValue = strdup(pszExpression+iValueIndex);
                        break;
                    }
                    else
                      pszAttributeName[iValue++] = pszExpression[i];
                }
                pszAttributeName[iValue] = '\0';
            }
        }
    }
    else if (bOneCharCompare == 0)
    {
        nLength = strlen(pszExpression);
        pszAttributeName = (char *)malloc(sizeof(char)*(nLength+1));
        iValue = 0;
        for (i=0; i<nLength-2; i++)
        {
            if ((pszExpression[i] != szCompare[0] || 
                 pszExpression[i] != toupper(szCompare[0])) &&
                (pszExpression[i] != szCompare2[0] || 
                 pszExpression[i] != toupper(szCompare2[0])))
                
            {
                pszAttributeName[iValue++] = pszExpression[i];
            }
            else
            {
                if (((pszExpression[i+1] == szCompare[1] || 
                     pszExpression[i+1] == toupper(szCompare[1])) ||
                    (pszExpression[i+1] == szCompare2[1] || 
                     pszExpression[i+1] == toupper(szCompare2[1]))) &&
                    (pszExpression[i+2] == ' '))
                {
                    iValueIndex = i+3;
                    pszAttributeValue = strdup(pszExpression+iValueIndex);
                    break;
                }
                else
                  pszAttributeName[iValue++] = pszExpression[i];
            }
            pszAttributeName[iValue] = '\0';
        }
    }

/* -------------------------------------------------------------------- */
/*      Return the name of the attribute : It is supposed to be         */
/*      inside []                                                       */
/* -------------------------------------------------------------------- */
    if (bReturnName)
    {
        if (!pszAttributeName)
          return NULL;

        nLength = strlen(pszAttributeName);
        pszFinalAttributeName = (char *)malloc(sizeof(char)*(nLength+1));
        bStartCopy= 0;
        iAtt = 0;
        for (i=0; i<nLength; i++)
        {
            if (pszAttributeName[i] == ' ' && bStartCopy == 0)
              continue;

            if (pszAttributeName[i] == '[')
            {
                bStartCopy = 1;
                continue;
            }
            if (pszAttributeName[i] == ']')
              break;
            if (bStartCopy)
            {
                pszFinalAttributeName[iAtt++] = pszAttributeName[i];
            }
            pszFinalAttributeName[iAtt] = '\0';
        }

        return pszFinalAttributeName;
    }
    else
    {
        
        if (!pszAttributeValue)
          return NULL;
        nLength = strlen(pszAttributeValue);
        pszFinalAttributeValue = (char *)malloc(sizeof(char)*(nLength+1));
        bStartCopy= 0;
        iAtt = 0;
        for (i=0; i<nLength; i++)
        {
            if (pszAttributeValue[i] == ' ' && bStartCopy == 0)
              continue;

            if (pszAttributeValue[i] == '\'' && bStartCopy == 0)
            {
                bSingleQuote = 1;
                bStartCopy = 1;
                continue;
            }
            else if (pszAttributeValue[i] == '"' && bStartCopy == 0)
            {
                bDoubleQuote = 1;
                bStartCopy = 1;
                continue;
            }
            else
              bStartCopy =1;

            if (bStartCopy)
            {
                if (pszAttributeValue[i] == '\'' && bSingleQuote)
                  break;
                else if (pszAttributeValue[i] == '"' && bDoubleQuote)
                  break;
                else if (pszAttributeValue[i] == ')')
                  break;
                pszFinalAttributeValue[iAtt++] = pszAttributeValue[i];
            }
            pszFinalAttributeValue[iAtt] = '\0';
        }
            
        return pszFinalAttributeValue;
    }
}


char *msSLDGetAttributeName(char *pszExpression, 
                         char *pszComparionValue)
{
    return msSLDGetAttributeNameOrValue(pszExpression, pszComparionValue, 1);
}

char *msSLDGetAttributeValue(char *pszExpression, 
                             char *pszComparionValue)
{
    return msSLDGetAttributeNameOrValue(pszExpression, pszComparionValue, 0);
}



/************************************************************************/
/*                           BuildExpressionTree                        */
/*                                                                      */
/*      Build a filter expression node based on mapserver's class       */
/*      expression. This is limited to simple expressions like :        */
/*        A = B, A < B, A <= B, A > B, A >= B, A != B                   */
/*       It also handles one level of logical expressions :             */
/*        A AND B                                                       */
/*        A OR B                                                        */
/*        NOT A                                                         */
/************************************************************************/
FilterEncodingNode *BuildExpressionTree(char *pszExpression, 
                                        FilterEncodingNode *psNode)
{
    char *apszExpression[20]; 
    int nLength = 0;
    /* int bInsideExpression = 0; */
    int i =0, nOperators=0;
    char *pszFinalExpression = NULL;
    int iFinal = 0, iIndiceExp=0, nOpeningBrackets=0;/* nIndice=0; */
    /* char szTmp[6]; */
    int iExpression = 0;
    /* char *pszSimplifiedExpression = NULL; */
    char *pszComparionValue=NULL, *pszAttibuteName=NULL;
    char *pszAttibuteValue=NULL;
    char *pszLeftExpression=NULL, *pszRightExpression=NULL, *pszOperator=NULL;

    if (!pszExpression || (nLength = strlen(pszExpression)) <=0)
      return NULL;

    for (i=0; i<20; i++)
      apszExpression[i] = (char *)malloc(sizeof(char)*(nLength+1));

    pszFinalExpression = (char *)malloc(sizeof(char)*(nLength+1));
    pszFinalExpression[0] = '\0';

    iExpression = -1; /* first incremnt will put it to 0; */
    iFinal = 0;
    iIndiceExp = 0;
    nOpeningBrackets = 0;

/* -------------------------------------------------------------------- */
/*      First we check how many logical operators are there :           */
/*       - if none : It means It is a coamrision operator (like =,      */
/*      >, >= .... We get the comparison value as well as the           */
/*      attribute and the attribut's value and assign it to the node    */
/*      passed in argument.                                             */
/*       - if there is one operator, we assign the operator to the      */
/*      node and adds the expressions into the left and right nodes.    */
/* -------------------------------------------------------------------- */
    nOperators = msSLDNumberOfLogicalOperators(pszExpression);
    if (nOperators == 0)
    {
        if (!psNode)
          psNode = FLTCreateFilterEncodingNode();

        pszComparionValue = msSLDGetComparisonValue(pszExpression);
        pszAttibuteName = msSLDGetAttributeName(pszExpression, pszComparionValue);
        pszAttibuteValue = msSLDGetAttributeValue(pszExpression, pszComparionValue);
        if (pszComparionValue && pszAttibuteName && pszAttibuteValue)
        {
            psNode->eType = FILTER_NODE_TYPE_COMPARISON;
            psNode->pszValue = strdup(pszComparionValue);

            psNode->psLeftNode = FLTCreateFilterEncodingNode();
            psNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
            psNode->psLeftNode->pszValue = strdup(pszAttibuteName);

            psNode->psRightNode = FLTCreateFilterEncodingNode();
            psNode->psRightNode->eType = FILTER_NODE_TYPE_LITERAL;
            psNode->psRightNode->pszValue = strdup(pszAttibuteValue);

            free(pszComparionValue);
            free(pszAttibuteName);
            free(pszAttibuteValue);
        }
        return psNode;
        
    }
    else if (nOperators == 1)
    {
        pszOperator = msSLDGetLogicalOperator(pszExpression);
        if (pszOperator)
        {
            if (!psNode)
                  psNode = FLTCreateFilterEncodingNode();

            psNode->eType = FILTER_NODE_TYPE_LOGICAL;
            psNode->pszValue = strdup(pszOperator);
            free(pszOperator);

            pszLeftExpression = msSLDGetLeftExpressionOfOperator(pszExpression);
            pszRightExpression = msSLDGetRightExpressionOfOperator(pszExpression);
            
            if (pszLeftExpression || pszRightExpression)
            {
                if (pszLeftExpression)
                {
                    pszComparionValue = msSLDGetComparisonValue(pszLeftExpression);
                    pszAttibuteName = msSLDGetAttributeName(pszLeftExpression, 
                                                            pszComparionValue);
                    pszAttibuteValue = msSLDGetAttributeValue(pszLeftExpression, 
                                                              pszComparionValue);

                    if (pszComparionValue && pszAttibuteName && pszAttibuteValue)
                    {
                        psNode->psLeftNode = FLTCreateFilterEncodingNode();
                        psNode->psLeftNode->eType = FILTER_NODE_TYPE_COMPARISON;
                        psNode->psLeftNode->pszValue = strdup(pszComparionValue);

                        psNode->psLeftNode->psLeftNode = FLTCreateFilterEncodingNode();
                        psNode->psLeftNode->psLeftNode->eType = 
                          FILTER_NODE_TYPE_PROPERTYNAME;
                        psNode->psLeftNode->psLeftNode->pszValue = strdup(pszAttibuteName);

                        psNode->psLeftNode->psRightNode = FLTCreateFilterEncodingNode();
                        psNode->psLeftNode->psRightNode->eType = 
                          FILTER_NODE_TYPE_LITERAL;
                        psNode->psLeftNode->psRightNode->pszValue = 
                          strdup(pszAttibuteValue);

                        free(pszComparionValue);
                        free(pszAttibuteName);
                        free(pszAttibuteValue);
                    }
                }
                if (pszRightExpression)
                {
                    pszComparionValue = msSLDGetComparisonValue(pszRightExpression);
                    pszAttibuteName = msSLDGetAttributeName(pszRightExpression, 
                                                            pszComparionValue);
                    pszAttibuteValue = msSLDGetAttributeValue(pszRightExpression, 
                                                              pszComparionValue);

                    if (pszComparionValue && pszAttibuteName && pszAttibuteValue)
                    {
                        psNode->psRightNode = FLTCreateFilterEncodingNode();
                        psNode->psRightNode->eType = FILTER_NODE_TYPE_COMPARISON;
                        psNode->psRightNode->pszValue = strdup(pszComparionValue);

                        psNode->psRightNode->psLeftNode = 
                          FLTCreateFilterEncodingNode();
                        psNode->psRightNode->psLeftNode->eType = 
                          FILTER_NODE_TYPE_PROPERTYNAME;
                        psNode->psRightNode->psLeftNode->pszValue = 
                          strdup(pszAttibuteName);

                        psNode->psRightNode->psRightNode = 
                          FLTCreateFilterEncodingNode();
                        psNode->psRightNode->psRightNode->eType = 
                          FILTER_NODE_TYPE_LITERAL;
                        psNode->psRightNode->psRightNode->pszValue = 
                          strdup(pszAttibuteValue);

                        free(pszComparionValue);
                        free(pszAttibuteName);
                        free(pszAttibuteValue);
                    }
                }
            }
        }

        return psNode;
    }
    else
      return NULL;

    /*
    for (i=0; i<nLength; i++)
    {
        if (pszExpression[i] == '(')
        {
            if (bInsideExpression)
            {
                pszFinalExpression[iFinal++] = pszExpression[i];
                nOpeningBrackets++;
            }
            else
            {
                bInsideExpression = 1;
                iExpression++;
                iIndiceExp = 0;
            }
        }
        else if (pszExpression[i] == ')')
        {
            if (bInsideExpression)
            {
                if (nOpeningBrackets > 0)
                {
                    nOpeningBrackets--;
                    apszExpression[iExpression][iIndiceExp++] = pszExpression[i];
                }
                else
                {
                    // end of an expression
                    pszFinalExpression[iFinal++] = ' ';
                    pszFinalExpression[iFinal] = '\0';
                    sprintf(szTmp, "exp%d ", iExpression);
                    strcat(pszFinalExpression,szTmp);
                    if (iExpression < 10)
                      iFinal+=5;
                    else
                      iFinal+=6;
                    bInsideExpression = 0;
                }
            }
        }
        else
        {
            if (bInsideExpression)
            {
                apszExpression[iExpression][iIndiceExp++] = pszExpression[i];
            }
            else
            {
                pszFinalExpression[iFinal++] =  pszExpression[i];
            }
        }

        if (iExpression >=0 && iIndiceExp >0)
          apszExpression[iExpression][iIndiceExp] = '\0';
        if (iFinal > 0)
          pszFinalExpression[iFinal] = '\0';
    }

    if (msSLDHasMoreThatOneLogicalOperator(pszFinalExpression))
    {
        pszSimplifiedExpression = 
          msSLDSimplifyExpression(pszFinalExpression);
        free(pszFinalExpression);
        
        // increase the size so it can fit the brakets () that will be added
        pszFinalExpression = (char *)malloc(sizeof(char)*(nLength+3));
        if(iExpression > 0)
        {
            nLength = strlen(pszSimplifiedExpression);
            iFinal = 0;
            for (i=0; i<nLength; i++)
            {
                if (i < nLength-4)
                {
                    if (pszSimplifiedExpression[i] == 'e' &&
                        pszSimplifiedExpression[i+1] == 'x' &&
                        pszSimplifiedExpression[i+2] == 'p')
                    {
                        nIndice = atoi(pszSimplifiedExpression[i+3]);
                        if (nIndice >=0 && nIndice < iExpression)
                        {
                            strcat(pszFinalExpression, 
                                   apszExpression[nIndice]);
                            iFinal+= strlen(apszExpression[nIndice]);
                        }
                    }
                }
                else
                {
                    pszFinalExpression[iFinal++] = pszSimplifiedExpression[i];
                }
            }    
        }
        else
          pszFinalExpression = strdup(pszFinalExpression);

        return BuildExpressionTree(pszFinalExpression, psNode);
    }
    pszLogicalOper = msSLDGetLogicalOperator(pszFinalExpression);
    //TODO : NOT operator
    if (pszLogicalOper)
    {
        if (strcasecmp(pszLogicalOper, "AND") == 0 ||
            strcasecmp(pszLogicalOper, "OR") == 0)
        {
            if (!psNode)
              psNode = FLTCreateFilterEncodingNode();
        
            psNode->eType = FILTER_NODE_TYPE_LOGICAL;
            if (strcasecmp(pszLogicalOper, "AND") == 0)
              psNode->pszValue = "AND";
            else
              psNode->pszValue = "OR";
                
            psNode->psLeftNode =  FLTCreateFilterEncodingNode();
            psNode->psRightNode =  FLTCreateFilterEncodingNode();

            psLeftExpresion = 
              msSLDGetLogicalOperatorExpression(pszFinalExpression, 0);
            psRightExpresion = 
              msSLDGetLogicalOperatorExpression(pszFinalExpression, 0);
            
            BuildExpressionTree(psNode->psLeftNode, psLeftExpresion);

            BuildExpressionTree(psNode->psRightNode,psRightExpresion);

            if (psLeftExpresion)
              free(psLeftExpresion);
            if (psRightExpresion)
              free(psRightExpresion);
        }
    }
    else //means it is a simple expression with comaprison
    {

    */
    
}   
    
char *msSLDBuildFilterEncoding(FilterEncodingNode *psNode)
{
    char *pszTmp = NULL;
    char szTmp[200];
    char *pszExpression = NULL;

    if (!psNode)
      return NULL;

    if (psNode->eType == FILTER_NODE_TYPE_COMPARISON && 
        psNode->pszValue && psNode->psLeftNode && psNode->psLeftNode->pszValue &&
        psNode->psRightNode && psNode->psRightNode->pszValue)
    {
        sprintf(szTmp,"<ogc:%s><ogc:PropertyName>%s</ogc:PropertyName><ogc:Literal>%s</ogc:Literal></ogc:%s>",
                psNode->pszValue, psNode->psLeftNode->pszValue,
                psNode->psRightNode->pszValue, psNode->pszValue);
        pszExpression = strdup(szTmp);
    }
    else if (psNode->eType == FILTER_NODE_TYPE_LOGICAL && 
             psNode->pszValue && 
             ((psNode->psLeftNode && psNode->psLeftNode->pszValue) ||
              (psNode->psRightNode && psNode->psRightNode->pszValue)))
    {
        sprintf(szTmp, "<ogc:%s>", psNode->pszValue);
        pszExpression = strcatalloc(pszExpression, szTmp);
        if (psNode->psLeftNode)
        {
            pszTmp = msSLDBuildFilterEncoding(psNode->psLeftNode);
            if (pszTmp)
            {
                pszExpression = strcatalloc(pszExpression, pszTmp); 
                free(pszTmp);
            }
        }
        if (psNode->psRightNode)
        {
            pszTmp = msSLDBuildFilterEncoding(psNode->psRightNode);
            if (pszTmp)
            {
                pszExpression = strcatalloc(pszExpression, pszTmp); 
                free(pszTmp);
            }
        }
        sprintf(szTmp, "</ogc:%s>", psNode->pszValue);
        pszExpression = strcatalloc(pszExpression, szTmp);
    }
    return pszExpression;
}

      

char *msSLDParseLogicalExpression(char *pszExpression, const char *pszWfsFilter)
{
    FilterEncodingNode *psNode = NULL;
    char *pszFLTExpression = NULL;
    char *pszTmp = NULL;

    if (!pszExpression || strlen(pszExpression) <=0)
      return NULL;


    /* psNode = BuildExpressionTree(pszExpression, NULL); */
    psNode = BuildExpressionTree(pszExpression, NULL);
    
    if (psNode)
    {
        pszFLTExpression = msSLDBuildFilterEncoding(psNode);
        if (pszFLTExpression)
        {
            pszTmp = strcatalloc(pszTmp, "<ogc:Filter>");
            if (pszWfsFilter)
            {
                pszTmp = strcatalloc(pszTmp, "<ogc:And>");
                pszTmp = strcatalloc(pszTmp, (char *)pszWfsFilter);
            }
            pszTmp = strcatalloc(pszTmp, pszFLTExpression);

            if (pszWfsFilter)
              pszTmp = strcatalloc(pszTmp, "</ogc:And>");

            pszTmp = strcatalloc(pszTmp, "</ogc:Filter>\n");

            free(pszFLTExpression);
            pszFLTExpression = pszTmp;
        }
            
    }
    
    return pszFLTExpression;
}

/************************************************************************/
/*             char *msSLDParseExpression(char *pszExpression)          */
/*                                                                      */
/*      Return an OGC filter for a mapserver locgical expression.       */
/*      TODO : move function to mapogcfilter.c                          */
/************************************************************************/
char *msSLDParseExpression(char *pszExpression)
{
    int nElements = 0;
    char **aszElements = NULL;
    char szBuffer[500];
    char szFinalAtt[40];
    char szFinalValue[40];
    char szAttribute[40];
    char szValue[40];
    int i=0, nLength=0, iAtt=0, iVal=0; 
    int bStartCopy=0, bSinglequote=0, bDoublequote=0;
    char *pszFilter = NULL;
 
    if (!pszExpression)
      return NULL;

    nLength = strlen(pszExpression);

    aszElements = split(pszExpression, ' ', &nElements);

    szFinalAtt[0] = '\0';
    szFinalValue[0] = '\0';
    for (i=0; i<nElements; i++)
    {
        if (strcasecmp(aszElements[i], "=") == 0 ||
            strcasecmp(aszElements[i], "eq") == 0)
        {
            if (i > 0 && i < nElements-1)
            {
                sprintf(szAttribute, aszElements[i-1]);
                sprintf(szValue, aszElements[i+1]);

                /* parse attribute */
                nLength = strlen(szAttribute);
                if (nLength > 0)
                {
                    iAtt = 0;
                    for (i=0; i<nLength; i++)
                    {
                        if (szAttribute[i] == '[')
                        {
                            bStartCopy = 1;
                            continue;
                        }
                        if (szAttribute[i] == ']')
                          break;
                        if (bStartCopy)
                        {
                            szFinalAtt[iAtt] = szAttribute[i];
                            iAtt++;
                        }
                        szFinalAtt[iAtt] = '\0';
                    }
                }

                /* parse value */
                nLength = strlen(szValue);
                if (nLength > 0)
                {
                    if (szValue[0] == '\'')
                      bSinglequote = 1;
                    else if (szValue[0] == '\"')
                       bDoublequote = 1;
                    else
                      sprintf(szFinalValue,szValue);
                    
                    iVal = 0;
                    if (bSinglequote || bDoublequote)
                    {
                         for (i=1; i<nLength-1; i++)
                           szFinalValue[iVal++] = szValue[i];

                         szFinalValue[iVal] = '\0';
                    }
                }
            }
            if (strlen(szFinalAtt) > 0 && strlen(szFinalValue) >0)
            {
                sprintf(szBuffer, "<ogc:Filter><ogc:PropertyIsEqualTo><ogc:PropertyName>%s</ogc:PropertyName><ogc:Literal>%s</ogc:Literal></ogc:PropertyIsEqualTo></ogc:Filter>", 
                        szFinalAtt, szFinalValue);
                pszFilter = strdup(szBuffer);
            }
        }
    }

    return pszFilter;
}               
               
    
/************************************************************************/
/*                     msSLDConvertRegexExpToOgcIsLike                  */
/*                                                                      */
/*      Convert mapserver regex expression to ogc is like propoery      */
/*      exprssion.                                                      */
/*                                                                      */
/*      Review bug 1644 for details. Here are the current rules:        */
/*                                                                      */
/*       The filter encoding property like is more limited compared     */
/*      to regular expressiosn that can be built in mapserver. I        */
/*      think we should define what is possible to convert properly     */
/*      and do those, and also identify potential problems.  Example :  */
/*        - any time there is a .* in the expression it will be         */
/*      converted to *                                                  */
/*        - any other character plus all the metacharcters . ^ $ * +    */
/*      ? { [ ] \ | ( ) would be outputed as is. (In case of            */
/*      mapserver, when we read the the ogc filter expression, we       */
/*      convert the wild card chracter to .*, and we convert the        */
/*      single chracter to .  and the escpae character to \ all         */
/*      other are outputed as is)                                       */
/*        - the  ogc tag would look like <ogc:PropertyIsLike            */
/*      wildCard="*"  singleChar="." escape="\">                        */
/*                                                                      */
/*        - type of potential problem :                                 */
/*           * if an expression is like /T (star)/ it will be           */
/*      converted to T* which is not correct.                           */
/*                                                                      */
/************************************************************************/
char *msSLDConvertRegexExpToOgcIsLike(char *pszRegex)
{   
    char szBuffer[1024];
    int iBuffer = 0, i=0;
    int nLength = 0;

    if (!pszRegex || strlen(pszRegex) == 0)
      return NULL;

    szBuffer[0] = '\0';
    nLength = strlen(pszRegex);

    while (i < nLength)
    {
        if (pszRegex[i] != '.')
        {
            szBuffer[iBuffer++] = pszRegex[i];
            i++;
        }
        else 
        {
            if (i<nLength-1 && pszRegex[i+1] == '*')
            {
                szBuffer[iBuffer++] = '*';
                i = i+2;
            }
            else
            {
                szBuffer[iBuffer++] =  pszRegex[i];
                i++;
            }
        }
    }
    szBuffer[iBuffer] = '\0';

    return strdup(szBuffer);
}



/************************************************************************/
/*                              msSLDGetFilter                          */
/*                                                                      */
/*      Get the correspondinf ogc Filter based on the class             */
/*      expression. TODO : move function to mapogcfilter.c when         */
/*      finished.                                                       */
/************************************************************************/
char *msSLDGetFilter(classObj *psClass, const char *pszWfsFilter)
{
    char *pszFilter = NULL;
    char szBuffer[500];
    char *pszOgcFilter = NULL;

    if (psClass && psClass->expression.string)
    {   
        /* string expression */
        if (psClass->expression.type == MS_STRING)
        {
            if (psClass->layer && psClass->layer->classitem)
            {
                if (pszWfsFilter)
                  sprintf(szBuffer, "<ogc:Filter><ogc:And>%s<ogc:PropertyIsEqualTo><ogc:PropertyName>%s</ogc:PropertyName><ogc:Literal>%s</ogc:Literal></ogc:PropertyIsEqualTo></ogc:And></ogc:Filter>\n", 
                        pszWfsFilter, psClass->layer->classitem, psClass->expression.string);
                else
                  sprintf(szBuffer, "<ogc:Filter><ogc:PropertyIsEqualTo><ogc:PropertyName>%s</ogc:PropertyName><ogc:Literal>%s</ogc:Literal></ogc:PropertyIsEqualTo></ogc:Filter>\n", 
                        psClass->layer->classitem, psClass->expression.string);
                pszFilter = strdup(szBuffer);
            }
        }
        else if (psClass->expression.type == MS_EXPRESSION)
        {
            pszFilter = msSLDParseLogicalExpression(psClass->expression.string, 
                                                    pszWfsFilter);
        }
        else if (psClass->expression.type == MS_REGEX)
        {
            if (psClass->layer && psClass->layer->classitem && psClass->expression.string)
            {
                pszOgcFilter = msSLDConvertRegexExpToOgcIsLike(psClass->expression.string);

                if (pszWfsFilter)
                  sprintf(szBuffer, "<ogc:Filter><ogc:And>%s<ogc:PropertyIsLike wildCard=\"*\" singleChar=\".\" escape=\"\\\"><ogc:PropertyName>%s</ogc:PropertyName><ogc:Literal>%s</ogc:Literal></ogc:PropertyIsLike></ogc:And></ogc:Filter>\n", 
                        pszWfsFilter, psClass->layer->classitem, pszOgcFilter);
                else
                  sprintf(szBuffer, "<ogc:Filter><ogc:PropertyIsLike wildCard=\"*\" singleChar=\".\" escape=\"\\\"><ogc:PropertyName>%s</ogc:PropertyName><ogc:Literal>%s</ogc:Literal></ogc:PropertyIsLike></ogc:Filter>\n", 
                          psClass->layer->classitem, pszOgcFilter);

                free(pszOgcFilter);

                pszFilter = strdup(szBuffer);
            }
        }
    }
    else if (pszWfsFilter)
    {
        sprintf(szBuffer, "<ogc:Filter>%s</ogc:Filter>\n", pszWfsFilter);
        pszFilter = strdup(szBuffer);
    }
    return pszFilter;
}            

#endif

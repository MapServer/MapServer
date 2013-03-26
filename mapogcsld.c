/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
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

#include "mapogcsld.h"
#include "mapogcfilter.h"
#include "mapserver.h"

#ifdef USE_OGR
#include "cpl_string.h"
#endif



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
                     char *pszStyleLayerName,  char **ppszLayerNames)
{
#ifdef USE_OGR

  /* needed for libcurl function msHTTPGetFile in maphttp.c */
#if defined(USE_CURL)

  char *pszSLDTmpFile = NULL;
  int status = 0;
  char *pszSLDbuf=NULL;
  FILE *fp = NULL;
  int nStatus = MS_FAILURE;

  if (map && szURL) {
    pszSLDTmpFile = msTmpFile(map, map->mappath, NULL, "sld.xml");
    if (pszSLDTmpFile == NULL) {
      pszSLDTmpFile = msTmpFile(map, NULL, NULL, "sld.xml" );
    }
    if (msHTTPGetFile(szURL, pszSLDTmpFile, &status,-1, 0, 0) ==  MS_SUCCESS) {
      if ((fp = fopen(pszSLDTmpFile, "rb")) != NULL) {
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
    } else {
      msSetError(MS_WMSERR, "Could not open SLD %s and save it in temporary file %s. Please make sure that the sld url is valid and that the temporary path is set. The temporary path can be defined for example by setting TMPPATH in the map file. Please check the MapServer documentation on temporary path settings.", "msSLDApplySLDURL", szURL, pszSLDTmpFile);
    }
    if (pszSLDbuf)
      nStatus = msSLDApplySLD(map, pszSLDbuf, iLayer, pszStyleLayerName, ppszLayerNames);
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
                  char *pszStyleLayerName, char **ppszLayerNames)
{
#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR) || defined(USE_SOS_SVR)

#ifdef USE_OGR

  int nLayers = 0;
  layerObj *pasLayers = NULL;
  int i, j, k, z, iClass;
  int bUseSpecificLayer = 0;
  const char *pszTmp = NULL;
  int bFreeTemplate = 0;
  int nLayerStatus = 0;
  int nStatus = MS_SUCCESS;
  /*const char *pszSLDNotSupported = NULL;*/
  char *tmpfilename = NULL;
  const char *pszFullName = NULL;
  char szTmp[512];
  char *pszTmp1=NULL;
  char *pszTmp2 = NULL;
  char *pszBuffer = NULL;
  layerObj *lp = NULL;
  char *pszSqlExpression=NULL;
  FilterEncodingNode *psExpressionNode =NULL;
  int bFailedExpression=0;

  pasLayers = msSLDParseSLD(map, psSLDXML, &nLayers);
  /* -------------------------------------------------------------------- */
  /*      If the same layer is given more that once, we need to           */
  /*      duplicate it.                                                   */
  /* -------------------------------------------------------------------- */
  if (pasLayers && nLayers>0) {
    int l,m;
    for (m=0; m<nLayers; m++) {
      layerObj *psTmpLayer=NULL;
      int nIndex;
      char tmpId[128];
      for (l=0; l<nLayers; l++) {
        if(pasLayers[m].name == NULL || pasLayers[l].name == NULL)
          continue;

        nIndex = msGetLayerIndex(map, pasLayers[m].name);

        if (m !=l && strcasecmp(pasLayers[m].name, pasLayers[l].name)== 0 &&
            nIndex != -1) {
          psTmpLayer = (layerObj *) malloc(sizeof(layerObj));
          initLayer(psTmpLayer, map);
          msCopyLayer(psTmpLayer, GET_LAYER(map,nIndex));
          /* open the source layer */
          if ( !psTmpLayer->vtable)
            msInitializeVirtualTable(psTmpLayer);

          /*make the name unique*/
          snprintf(tmpId, sizeof(tmpId), "%lx_%x_%d",(long)time(NULL),(int)getpid(),
                   map->numlayers);
          if (psTmpLayer->name)
            msFree(psTmpLayer->name);
          psTmpLayer->name = strdup(tmpId);
          msFree(pasLayers[l].name);
          pasLayers[l].name = strdup(tmpId);
          msInsertLayer(map, psTmpLayer, -1);
          MS_REFCNT_DECR(psTmpLayer);
        }
      }
    }
  }

  if (pasLayers && nLayers > 0) {
    for (i=0; i<map->numlayers; i++) {
      if (iLayer >=0 && iLayer< map->numlayers) {
        i = iLayer;
        bUseSpecificLayer = 1;
      }

      /* compare layer name to wms_name as well */
      pszTmp = msOWSLookupMetadata(&(GET_LAYER(map, i)->metadata), "MO", "name");

      for (j=0; j<nLayers; j++) {
        /* -------------------------------------------------------------------- */
        /*      copy :  - class                                                 */
        /*              - layer's labelitem                                     */
        /* -------------------------------------------------------------------- */
        if ((pasLayers[j].name && pszStyleLayerName == NULL &&
             ((strcasecmp(GET_LAYER(map, i)->name, pasLayers[j].name) == 0 ||
               (pszTmp && strcasecmp(pszTmp, pasLayers[j].name) == 0))||
              (GET_LAYER(map, i)->group &&
               strcasecmp(GET_LAYER(map, i)->group, pasLayers[j].name) == 0))) ||
            (bUseSpecificLayer && pszStyleLayerName && pasLayers[j].name &&
             strcasecmp(pasLayers[j].name, pszStyleLayerName) == 0)) {
#ifdef notdef
          /*this is a test code if we decide to flag some layers as not supporting SLD*/
          pszSLDNotSupported = msOWSLookupMetadata(&(GET_LAYER(map, i)->metadata), "M", "SLD_NOT_SUPPORTED");
          if (pszSLDNotSupported) {
            msSetError(MS_WMSERR, "Layer %s does not support SLD", "msSLDApplySLD", pasLayers[j].name);
            return MS_FAILURE;
          }
#endif

          if ( pasLayers[j].numclasses > 0) {
            GET_LAYER(map, i)->type = pasLayers[j].type;

            for(k=0; k<GET_LAYER(map, i)->numclasses; k++) {
              if (GET_LAYER(map, i)->class[k] != NULL) {
                GET_LAYER(map, i)->class[k]->layer=NULL;
                if (freeClass(GET_LAYER(map, i)->class[k]) == MS_SUCCESS ) {
                  msFree(GET_LAYER(map, i)->class[k]);
                  GET_LAYER(map, i)->class[k] = NULL;
                }
              }
            }

            GET_LAYER(map, i)->numclasses = 0;

            /*unset the classgroup on the layer if it was set. This allows the layer to render
              with all the classes defined in the SLD*/
            msFree(GET_LAYER(map, i)->classgroup);
            GET_LAYER(map, i)->classgroup = NULL;

            iClass = 0;
            for (k=0; k < pasLayers[j].numclasses; k++) {
              if (msGrowLayerClasses(GET_LAYER(map, i)) == NULL)
                return MS_FAILURE;

              initClass(GET_LAYER(map, i)->class[iClass]);
              msCopyClass(GET_LAYER(map, i)->class[iClass],
                          pasLayers[j].class[k], NULL);
              GET_LAYER(map, i)->class[iClass]->layer = GET_LAYER(map, i);
              GET_LAYER(map, i)->class[iClass]->type = GET_LAYER(map, i)->type;
              GET_LAYER(map, i)->numclasses++;

              /*aliases may have been used as part of the sld text symbolizer for
                label element. Try to process it if that is the case #3114*/
              if (msLayerOpen(GET_LAYER(map, i)) == MS_SUCCESS &&
                  msLayerGetItems(GET_LAYER(map, i)) == MS_SUCCESS) {
                if (GET_LAYER(map, i)->class[iClass]->text.string) {
                  for(z=0; z<GET_LAYER(map, i)->numitems; z++) {
                    if (!GET_LAYER(map, i)->items[z] || strlen(GET_LAYER(map, i)->items[z]) <= 0)
                      continue;
                    snprintf(szTmp, sizeof(szTmp), "%s_alias", GET_LAYER(map, i)->items[z]);
                    pszFullName = msOWSLookupMetadata(&(GET_LAYER(map, i)->metadata), "G", szTmp);
                    pszTmp1 = msStrdup( GET_LAYER(map, i)->class[iClass]->text.string);
                    if (pszFullName != NULL && (strstr(pszTmp1, pszFullName) != NULL)) {
                      char *tmpstr1= NULL;
                      tmpstr1 = msReplaceSubstring(pszTmp1, pszFullName, GET_LAYER(map, i)->items[z]);
                      pszTmp2 = (char *)malloc(sizeof(char)*(strlen(tmpstr1)+3));
                      sprintf(pszTmp2,"(%s)",tmpstr1);
                      msLoadExpressionString(&(GET_LAYER(map, i)->class[iClass]->text), pszTmp2);
                      msFree(pszTmp2);
                    }
                    msFree(pszTmp1);
                  }
                }
              }

              iClass++;
            }
          } else {
            /*this is probably an SLD that uses Named styles*/
            if (pasLayers[j].classgroup) {
              for (k=0; k<GET_LAYER(map, i)->numclasses; k++) {
                if (GET_LAYER(map, i)->class[k]->group &&
                    strcasecmp(GET_LAYER(map, i)->class[k]->group,
                               pasLayers[j].classgroup) == 0)
                  break;
              }
              if (k < GET_LAYER(map, i)->numclasses) {
                msFree( GET_LAYER(map, i)->classgroup);
                GET_LAYER(map, i)->classgroup = msStrdup(pasLayers[j].classgroup);
              } else {
                /* TODO  we throw an exception ?*/
              }
            }
          }
          if (pasLayers[j].labelitem) {
            if (GET_LAYER(map, i)->labelitem)
              free(GET_LAYER(map, i)->labelitem);

            GET_LAYER(map, i)->labelitem = msStrdup(pasLayers[j].labelitem);
          }

          if (pasLayers[j].classitem) {
            if (GET_LAYER(map, i)->classitem)
              free(GET_LAYER(map, i)->classitem);

            GET_LAYER(map, i)->classitem = msStrdup(pasLayers[j].classitem);
          }

          /* opacity for sld raster */
          if (GET_LAYER(map, i)->type == MS_LAYER_RASTER &&
              pasLayers[j].opacity != -1)
            GET_LAYER(map, i)->opacity = pasLayers[j].opacity;

          /* mark as auto-generate SLD */
          if (GET_LAYER(map, i)->connectiontype == MS_WMS)
            msInsertHashTable(&(GET_LAYER(map, i)->metadata),
                              "wms_sld_body", "auto" );
          /* ==================================================================== */
          /*      if the SLD contained a spatial feature, the layerinfo           */
          /*      parameter contains the node. Extract it and do a query on       */
          /*      the layer. Insert also a metadata that will be used when        */
          /*      rendering the final image.                                      */
          /* ==================================================================== */
          if (pasLayers[j].layerinfo &&
              (GET_LAYER(map, i)->type ==  MS_LAYER_POINT ||
               GET_LAYER(map, i)->type == MS_LAYER_LINE ||
               GET_LAYER(map, i)->type == MS_LAYER_POLYGON ||
               GET_LAYER(map, i)->type == MS_LAYER_ANNOTATION ||
               GET_LAYER(map, i)->type == MS_LAYER_TILEINDEX)) {
            FilterEncodingNode *psNode = NULL;

            msInsertHashTable(&(GET_LAYER(map, i)->metadata),
                              "tmp_wms_sld_query", "true" );
            psNode = (FilterEncodingNode *)pasLayers[j].layerinfo;

            /* -------------------------------------------------------------------- */
            /*      set the template on the classes so that the query works         */
            /*      using classes. If there are no classes, set it at the layer level.*/
            /* -------------------------------------------------------------------- */
            if (GET_LAYER(map, i)->numclasses > 0) {
              for (k=0; k<GET_LAYER(map, i)->numclasses; k++) {
                if (!GET_LAYER(map, i)->class[k]->template)
                  GET_LAYER(map, i)->class[k]->template = msStrdup("ttt.html");
              }
            } else if (!GET_LAYER(map, i)->template) {
              bFreeTemplate = 1;
              GET_LAYER(map, i)->template = msStrdup("ttt.html");
            }

            nLayerStatus =  GET_LAYER(map, i)->status;
            GET_LAYER(map, i)->status = MS_ON;

            nStatus =
            FLTApplyFilterToLayer(psNode, map,
                                  GET_LAYER(map, i)->index);
            /* -------------------------------------------------------------------- */
            /*      nothing found is a valid, do not exit.                          */
            /* -------------------------------------------------------------------- */
            if (nStatus !=  MS_SUCCESS) {
              errorObj   *ms_error;
              ms_error = msGetErrorObj();
              if(ms_error->code == MS_NOTFOUND)
                nStatus =  MS_SUCCESS;
            }


            GET_LAYER(map, i)->status = nLayerStatus;
            FLTFreeFilterEncodingNode(psNode);

            if ( bFreeTemplate) {
              free(GET_LAYER(map, i)->template);
              GET_LAYER(map, i)->template = NULL;
            }

            pasLayers[j].layerinfo=NULL;

            if( nStatus != MS_SUCCESS )
              return nStatus;
          } else {
            /*in some cases it would make sense to concatenate all the class
              expressions and use it to set the filter on the layer. This
              could increase performace. Will do it for db types layers #2840*/
            lp = GET_LAYER(map, i);
            if (lp->filter.string == NULL ||
                (lp->filter.string && lp->filter.type == MS_EXPRESSION)) {
              if (lp->connectiontype == MS_POSTGIS || lp->connectiontype ==  MS_ORACLESPATIAL ||
                  lp->connectiontype == MS_SDE || lp->connectiontype == MS_PLUGIN) {
                if (lp->numclasses > 0) {
                  /*check first that all classes have an expression type. That is
                    the only way we can concatenate them and set the filter
                    expression*/
                  for (k=0; k<lp->numclasses; k++) {
                    if (lp->class[k]->expression.type != MS_EXPRESSION)
                      break;
                  }
                  if (k == lp->numclasses) {
                    bFailedExpression = 0;
                    for (k=0; k<lp->numclasses; k++) {
                      if (pszBuffer == NULL)
                        snprintf(szTmp, sizeof(szTmp), "%s", "((");
                      else
                        snprintf(szTmp, sizeof(szTmp), "%s", " OR ");

                      pszBuffer =msStringConcatenate(pszBuffer, szTmp);
                      psExpressionNode = BuildExpressionTree(lp->class[k]->expression.string,NULL);
                      if (psExpressionNode) {
                        pszSqlExpression = FLTGetSQLExpression(psExpressionNode,lp);
                        if (pszSqlExpression) {
                          pszBuffer =
                            msStringConcatenate(pszBuffer, pszSqlExpression);
                          msFree(pszSqlExpression);
                        } else {
                          bFailedExpression =1;
                          break;
                        }
                        FLTFreeFilterEncodingNode(psExpressionNode);
                      } else {
                        bFailedExpression =1;
                        break;
                      }
                    }
                    if (!bFailedExpression) {
                      snprintf(szTmp, sizeof(szTmp), "%s", "))");
                      pszBuffer =msStringConcatenate(pszBuffer, szTmp);
                      msLoadExpressionString(&lp->filter, pszBuffer);
                    }
                    msFree(pszBuffer);
                    pszBuffer = NULL;
                  }
                }
              }
            }

          }
          break;
        }
      }
      if (bUseSpecificLayer)
        break;
    }

    /* -------------------------------------------------------------------- */
    /*      if needed return a comma separated list of the layers found     */
    /*      in the sld.                                                     */
    /* -------------------------------------------------------------------- */
    if (ppszLayerNames) {
      char *pszTmp = NULL;
      for (i=0; i<nLayers; i++) {
        if (pasLayers[i].name) {
          if (pszTmp !=NULL)
            pszTmp = msStringConcatenate(pszTmp, ",");
          pszTmp = msStringConcatenate(pszTmp, pasLayers[i].name);

        }
      }
      *ppszLayerNames = pszTmp;

    }
    for (i=0; i<nLayers; i++)
      freeLayer(&pasLayers[i]);
    msFree(pasLayers);
  }
  if(map->debug == MS_DEBUGLEVEL_VVV) {
    tmpfilename = msTmpFile(map, map->mappath, NULL, "_sld.map");
    if (tmpfilename == NULL) {
      tmpfilename = msTmpFile(map, NULL, NULL, "_sld.map" );
    }
    if (tmpfilename) {
      msSaveMap(map,tmpfilename);
      msDebug("msApplySLD(): Map file after SLD was applied %s", tmpfilename);
      msFree(tmpfilename);
    }
  }
  return MS_SUCCESS;


#else
  /* ------------------------------------------------------------------
   * OGR Support not included...
   * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", "msSLDApplySLD()");
  return(MS_FAILURE);

#endif /* USE_OGR */

#else
  msSetError(MS_MISCERR, "OWS support is not available.",
             "msSLDApplySLD()");
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
      (strstr(psSLDXML, "StyledLayerDescriptor") == NULL)) {
    msSetError(MS_WMSERR, "Invalid SLD document", "");
    return NULL;
  }

  psRoot = CPLParseXMLString(psSLDXML);
  if( psRoot == NULL) {
    msSetError(MS_WMSERR, "Invalid SLD document : %s", "", psSLDXML);
    return NULL;
  }

  /* strip namespaces ogc and sld and gml */
  CPLStripXMLNamespace(psRoot, "ogc", 1);
  CPLStripXMLNamespace(psRoot, "sld", 1);
  CPLStripXMLNamespace(psRoot, "gml", 1);
  CPLStripXMLNamespace(psRoot, "se", 1);


  /* -------------------------------------------------------------------- */
  /*      get the root element (Filter).                                  */
  /* -------------------------------------------------------------------- */
  psChild = psRoot;
  psSLD = NULL;

  while( psChild != NULL ) {
    if (psChild->eType == CXT_Element &&
        EQUAL(psChild->pszValue,"StyledLayerDescriptor")) {
      psSLD = psChild;
      break;
    } else
      psChild = psChild->psNext;
  }

  if (!psSLD) {
    msSetError(MS_WMSERR, "Invalid SLD document : %s", "", psSLDXML);
    return NULL;
  }

  /* -------------------------------------------------------------------- */
  /*      Parse the named layers.                                         */
  /* -------------------------------------------------------------------- */
  psNamedLayer = CPLGetXMLNode(psSLD, "NamedLayer");
  while (psNamedLayer) {
    if (!psNamedLayer->pszValue ||
        strcasecmp(psNamedLayer->pszValue, "NamedLayer") != 0) {
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
  if (psNamedLayer) {
    iLayer = 0;
    while (psNamedLayer)

    {
      if (!psNamedLayer->pszValue ||
          strcasecmp(psNamedLayer->pszValue, "NamedLayer") != 0) {
        psNamedLayer = psNamedLayer->psNext;
        continue;
      }

      psName = CPLGetXMLNode(psNamedLayer, "Name");
      initLayer(&pasLayers[iLayer], map);

      if (psName && psName->psChild &&  psName->psChild->pszValue)
        pasLayers[iLayer].name = msStrdup(psName->psChild->pszValue);

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


int _msSLDParseSizeParameter(CPLXMLNode *psSize)
{
  int nSize = 0;
  CPLXMLNode *psLiteral = NULL;

  if (psSize) {
    psLiteral = CPLGetXMLNode(psSize, "Literal");
    if (psLiteral && psLiteral->psChild && psLiteral->psChild->pszValue)
      nSize = atof(psLiteral->psChild->pszValue);
    else if (psSize->psChild && psSize->psChild->pszValue)
      nSize = atof(psSize->psChild->pszValue);
  }

  return nSize;
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

  if (psRule && psLayer && nNewClasses > 0) {
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
    if (dfMinScale > 0 || dfMaxScale > 0) {
      for (i=0; i<nNewClasses; i++) {
        if (dfMinScale > 0)
          psLayer->class[psLayer->numclasses-1-i]->minscaledenom = dfMinScale;
        if (dfMaxScale)
          psLayer->class[psLayer->numclasses-1-i]->maxscaledenom = dfMaxScale;
      }
    }
    /* -------------------------------------------------------------------- */
    /*      set name and title to the classes created by the rule.          */
    /* -------------------------------------------------------------------- */
    for (i=0; i<nNewClasses; i++) {
      if (!psLayer->class[psLayer->numclasses-1-i]->name) {
        if (pszName)
          psLayer->class[psLayer->numclasses-1-i]->name = msStrdup(pszName);
        else if (pszTitle)
          psLayer->class[psLayer->numclasses-1-i]->name = msStrdup(pszTitle);
        else
          psLayer->class[psLayer->numclasses-1-i]->name = msStrdup("Unknown");
      }
    }
    if (pszTitle) {
      for (i=0; i<nNewClasses; i++) {
        psLayer->class[psLayer->numclasses-1-i]->title =
              msStrdup(pszTitle);
      }
    }

  }

}


/************************************************************************/
/*                           msSLDParseNamedLayer                       */
/*                                                                      */
/*      Parse NamedLayer root.                                          */
/************************************************************************/
int msSLDParseNamedLayer(CPLXMLNode *psRoot, layerObj *psLayer)
{
  CPLXMLNode *psFeatureTypeStyle, *psRule, *psUserStyle;
  CPLXMLNode *psSLDName = NULL, *psNamedStyle=NULL;
  CPLXMLNode *psElseFilter = NULL, *psFilter=NULL;
  CPLXMLNode *psTmpNode = NULL;
  FilterEncodingNode *psNode = NULL;
  int nNewClasses=0, nClassBeforeFilter=0, nClassAfterFilter=0;
  int nClassAfterRule=0, nClassBeforeRule=0;
  char *pszTmpFilter = NULL;
  layerObj *psCurrentLayer = NULL;
  const char *pszWmsName=NULL;
  int j=0;
  const char *key=NULL;

  if (!psRoot || !psLayer)
    return MS_FAILURE;

  psUserStyle = CPLGetXMLNode(psRoot, "UserStyle");
  if (psUserStyle) {
    psFeatureTypeStyle = CPLGetXMLNode(psUserStyle, "FeatureTypeStyle");
    if (psFeatureTypeStyle) {
      while (psFeatureTypeStyle && psFeatureTypeStyle->pszValue &&
             strcasecmp(psFeatureTypeStyle->pszValue,
                        "FeatureTypeStyle") == 0) {
        if (!psFeatureTypeStyle->pszValue ||
            strcasecmp(psFeatureTypeStyle->pszValue,
                       "FeatureTypeStyle") != 0) {
          psFeatureTypeStyle = psFeatureTypeStyle->psNext;
          continue;
        }

        /* -------------------------------------------------------------------- */
        /*      Parse rules with no Else filter.                                */
        /* -------------------------------------------------------------------- */
        psRule = CPLGetXMLNode(psFeatureTypeStyle, "Rule");
        while (psRule) {
          if (!psRule->pszValue ||
              strcasecmp(psRule->pszValue, "Rule") != 0) {
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
              psFilter->psChild->pszValue) {
            CPLXMLNode *psTmpNextNode = NULL;
            /* clone the tree and set the next node to null */
            /* so we only have the Filter node */
            psTmpNode = CPLCloneXMLTree(psFilter);
            psTmpNextNode = psTmpNode->psNext;
            psTmpNode->psNext = NULL;
            pszTmpFilter = CPLSerializeXMLTree(psTmpNode);
            psTmpNode->psNext = psTmpNextNode;
            CPLDestroyXMLNode(psTmpNode);

            if (pszTmpFilter) {
              /* nTmp = strlen(psFilter->psChild->pszValue)+17; */
              /* pszTmpFilter = malloc(sizeof(char)*nTmp); */
              /* sprintf(pszTmpFilter,"<Filter>%s</Filter>", */
              /* psFilter->psChild->pszValue); */
              /* pszTmpFilter[nTmp-1]='\0'; */
              psNode = FLTParseFilterEncoding(pszTmpFilter);

              CPLFree(pszTmpFilter);
            }

            if (psNode) {
              char *pszExpression = NULL;
              int i;

              /*preparse the filter for possible gml aliases set on the layer's metada:
                "gml_NA3DESC_alias" "alias_name" and filter could be
              <ogc:PropertyName>alias_name</ogc:PropertyName> #3079*/
              for (j=0; j<psLayer->map->numlayers; j++) {
                psCurrentLayer = GET_LAYER(psLayer->map, j);

                pszWmsName = msOWSLookupMetadata(&(psCurrentLayer->metadata), "MO", "name");

                if ((psCurrentLayer->name && psLayer->name &&
                     strcasecmp(psCurrentLayer->name, psLayer->name) == 0) ||
                    (psCurrentLayer->group && psLayer->name &&
                     strcasecmp(psCurrentLayer->group, psLayer->name) == 0) ||
                    (psLayer->name && pszWmsName &&
                     strcasecmp(pszWmsName, psLayer->name) == 0))
                  break;
              }
              if (j < psLayer->map->numlayers) {
                /*make sure that the tmp layer has all the metadata that
                  the orinal layer has, allowing to do parsing for
                  such things as gml_attribute_type #3052*/
                while (1) {
                  key = msNextKeyFromHashTable(&psCurrentLayer->metadata, key);
                  if (!key)
                    break;
                  else
                    msInsertHashTable(&psLayer->metadata, key,
                                      msLookupHashTable(&psCurrentLayer->metadata, key));
                }
                FLTPreParseFilterForAlias(psNode, psLayer->map, j, "G");
              }

              pszExpression = FLTGetCommonExpression(psNode, psLayer);
              FLTFreeFilterEncodingNode(psNode);
              psNode = NULL;

              if (pszExpression) {
                nNewClasses =
                  nClassAfterFilter - nClassBeforeFilter;
                for (i=0; i<nNewClasses; i++) {
                  msLoadExpressionString(&psLayer->
                                         class[psLayer->numclasses-1-i]->
                                         expression, pszExpression);
                }
                msFree(pszExpression);
                pszExpression = NULL;
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
        while (psRule) {
          if (!psRule->pszValue ||
              strcasecmp(psRule->pszValue, "Rule") != 0) {
            psRule = psRule->psNext;
            continue;
          }
          psElseFilter = CPLGetXMLNode(psRule, "ElseFilter");
          if (psElseFilter) {
            msSLDParseRule(psRule, psLayer);
            _SLDApplyRuleValues(psRule, psLayer, 1);
          }
          psRule = psRule->psNext;


        }

        psFeatureTypeStyle = psFeatureTypeStyle->psNext;
      }
    }
  }
  /* check for Named styles*/
  else {
    psNamedStyle = CPLGetXMLNode(psRoot, "NamedStyle");
    if (psNamedStyle) {
      psSLDName = CPLGetXMLNode(psNamedStyle, "Name");
      if (psSLDName && psSLDName->psChild &&  psSLDName->psChild->pszValue) {
        msFree(psLayer->classgroup);
        psLayer->classgroup = msStrdup(psSLDName->psChild->pszValue);
      }
    }
  }

  return MS_SUCCESS;
}


/************************************************************************/
/*        void msSLDParseRule(CPLXMLNode *psRoot, layerObj *psLayer)    */
/*                                                                      */
/*      Parse a Rule node into classes for a spcific layer.             */
/************************************************************************/
int msSLDParseRule(CPLXMLNode *psRoot, layerObj *psLayer)
{
  CPLXMLNode *psLineSymbolizer = NULL;
  CPLXMLNode *psPolygonSymbolizer = NULL;
  CPLXMLNode *psPointSymbolizer = NULL;
  CPLXMLNode *psTextSymbolizer = NULL;
  CPLXMLNode *psRasterSymbolizer = NULL;

  int bSymbolizer = 0;
  int bNewClass=0, nSymbolizer=0;

  if (!psRoot || !psLayer)
    return MS_FAILURE;

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
  while (psLineSymbolizer) {
    if (!psLineSymbolizer->pszValue ||
        strcasecmp(psLineSymbolizer->pszValue,
                   "LineSymbolizer") != 0) {
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
  while (psPolygonSymbolizer) {
    if (!psPolygonSymbolizer->pszValue ||
        strcasecmp(psPolygonSymbolizer->pszValue,
                   "PolygonSymbolizer") != 0) {
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
  while (psPointSymbolizer) {
    if (!psPointSymbolizer->pszValue ||
        strcasecmp(psPointSymbolizer->pszValue,
                   "PointSymbolizer") != 0) {
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
                    "TextSymbolizer") == 0) {
    if (!psTextSymbolizer->pszValue ||
        strcasecmp(psTextSymbolizer->pszValue,
                   "TextSymbolizer") != 0) {
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
                    "RasterSymbolizer") == 0) {
    if (!psRasterSymbolizer->pszValue ||
        strcasecmp(psRasterSymbolizer->pszValue,
                   "RasterSymbolizer") != 0) {
      psRasterSymbolizer = psRasterSymbolizer->psNext;
      continue;
    }
    msSLDParseRasterSymbolizer(psRasterSymbolizer, psLayer);
    psRasterSymbolizer = psRasterSymbolizer->psNext;
    psLayer->type = MS_LAYER_RASTER;
  }

  return MS_SUCCESS;
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
int msSLDParseLineSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer,
                             int bNewClass)
{
  int nClassId = 0;
  CPLXMLNode *psStroke=NULL, *psOffset=NULL;
  int iStyle = 0;

  if (!psRoot || !psLayer)
    return MS_FAILURE;

  psStroke =  CPLGetXMLNode(psRoot, "Stroke");
  if (psStroke) {
    if (bNewClass || psLayer->numclasses <= 0) {
      if (msGrowLayerClasses(psLayer) == NULL)
        return MS_FAILURE;
      initClass(psLayer->class[psLayer->numclasses]);
      nClassId = psLayer->numclasses;
      psLayer->numclasses++;
    } else
      nClassId = psLayer->numclasses-1;

    iStyle = psLayer->class[nClassId]->numstyles;
    msMaybeAllocateClassStyle(psLayer->class[nClassId], iStyle);

    msSLDParseStroke(psStroke, psLayer->class[nClassId]->styles[iStyle],
                     psLayer->map, 0);

    /*parse PerpendicularOffset SLD 1.1.10*/
    psOffset = CPLGetXMLNode(psRoot, "PerpendicularOffset");
    if (psOffset && psOffset->psChild && psOffset->psChild->pszValue) {
      psLayer->class[nClassId]->styles[iStyle]->offsetx = atoi(psOffset->psChild->pszValue);
      psLayer->class[nClassId]->styles[iStyle]->offsety = psLayer->class[nClassId]->styles[iStyle]->offsetx;
    }
  }

  return MS_SUCCESS;
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
/************************************************************************/
int msSLDParseStroke(CPLXMLNode *psStroke, styleObj *psStyle,
                     mapObj *map, int iColorParam)
{
  CPLXMLNode *psCssParam = NULL, *psGraphicFill=NULL;
  char *psStrkName = NULL;
  char *psColor = NULL;
  int nLength = 0;
  char *pszDashValue = NULL;

  if (!psStroke || !psStyle)
    return MS_FAILURE;

  /* parse css parameters */
  psCssParam =  CPLGetXMLNode(psStroke, "CssParameter");
  /*sld 1.1 used SvgParameter*/
  if (psCssParam == NULL)
    psCssParam =  CPLGetXMLNode(psStroke, "SvgParameter");

  while (psCssParam && psCssParam->pszValue &&
         (strcasecmp(psCssParam->pszValue, "CssParameter") == 0 ||
          strcasecmp(psCssParam->pszValue, "SvgParameter") == 0)) {
    psStrkName = (char*)CPLGetXMLValue(psCssParam, "name", NULL);

    if (psStrkName) {
      if (strcasecmp(psStrkName, "stroke") == 0) {
        if(psCssParam->psChild && psCssParam->psChild->psNext &&
            psCssParam->psChild->psNext->pszValue)
          psColor = psCssParam->psChild->psNext->pszValue;

        if (psColor) {
          nLength = strlen(psColor);
          /* expecting hexadecimal ex : #aaaaff */
          if (nLength == 7 && psColor[0] == '#') {
            if (iColorParam == 0) {
              psStyle->color.red = msHexToInt(psColor+1);
              psStyle->color.green = msHexToInt(psColor+3);
              psStyle->color.blue= msHexToInt(psColor+5);
            } else if (iColorParam == 1) {
              psStyle->outlinecolor.red = msHexToInt(psColor+1);
              psStyle->outlinecolor.green = msHexToInt(psColor+3);
              psStyle->outlinecolor.blue= msHexToInt(psColor+5);
            } else if (iColorParam == 2) {
              psStyle->backgroundcolor.red = msHexToInt(psColor+1);
              psStyle->backgroundcolor.green = msHexToInt(psColor+3);
              psStyle->backgroundcolor.blue= msHexToInt(psColor+5);
            }
          }
        }
      } else if (strcasecmp(psStrkName, "stroke-width") == 0) {
        if(psCssParam->psChild &&  psCssParam->psChild->psNext &&
            psCssParam->psChild->psNext->pszValue) {
          psStyle->width =
            atof(psCssParam->psChild->psNext->pszValue);
        }
      } else if (strcasecmp(psStrkName, "stroke-dasharray") == 0) {
        if(psCssParam->psChild && psCssParam->psChild->psNext &&
            psCssParam->psChild->psNext->pszValue) {
          int nDash = 0, i;
          char **aszValues = NULL;
          int nMaxDash;
          pszDashValue =
            msStrdup(psCssParam->psChild->psNext->pszValue);
          aszValues = msStringSplit(pszDashValue, ' ', &nDash);
          if (nDash > 0) {
            nMaxDash = nDash;
            if (nDash > MS_MAXPATTERNLENGTH)
              nMaxDash =  MS_MAXPATTERNLENGTH;

            psStyle->patternlength = nMaxDash;
            for (i=0; i<nMaxDash; i++)
              psStyle->pattern[i] = atof(aszValues[i]);

            msFreeCharArray(aszValues, nDash);
            psStyle->linecap = MS_CJC_BUTT;
          }

        }
      } else if (strcasecmp(psStrkName, "stroke-opacity") == 0) {
        if(psCssParam->psChild &&  psCssParam->psChild->psNext &&
            psCssParam->psChild->psNext->pszValue) {
          if (iColorParam == 0) {
            psStyle->color.alpha =
              (int)(atof(psCssParam->psChild->psNext->pszValue)*255);
          } else {
            psStyle->outlinecolor.alpha =
              (int)(atof(psCssParam->psChild->psNext->pszValue)*255);
          }
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

  return MS_SUCCESS;
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
int msSLDParsePolygonSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer,
                                int bNewClass)
{
  CPLXMLNode *psFill, *psStroke;
  int nClassId=0, iStyle=0;
  CPLXMLNode *psDisplacement=NULL, *psDisplacementX=NULL, *psDisplacementY=NULL;
  int nOffsetX=-1, nOffsetY=-1;

  if (!psRoot || !psLayer)
    return MS_FAILURE;

  /*parse displacement for SLD 1.1.0*/
  psDisplacement = CPLGetXMLNode(psRoot, "Displacement");
  if (psDisplacement) {
    psDisplacementX = CPLGetXMLNode(psDisplacement, "DisplacementX");
    psDisplacementY = CPLGetXMLNode(psDisplacement, "DisplacementY");
    /* psCssParam->psChild->psNext->pszValue) */
    if (psDisplacementX &&
        psDisplacementX->psChild &&
        psDisplacementX->psChild->pszValue &&
        psDisplacementY &&
        psDisplacementY->psChild &&
        psDisplacementY->psChild->pszValue) {
      nOffsetX = atoi(psDisplacementX->psChild->pszValue);
      nOffsetY = atoi(psDisplacementY->psChild->pszValue);
    }
  }

  psFill =  CPLGetXMLNode(psRoot, "Fill");
  if (psFill) {
    if (bNewClass || psLayer->numclasses <= 0) {
      if (msGrowLayerClasses(psLayer) == NULL)
        return MS_FAILURE;
      initClass(psLayer->class[psLayer->numclasses]);
      nClassId = psLayer->numclasses;
      psLayer->numclasses++;
    } else
      nClassId = psLayer->numclasses-1;

    iStyle = psLayer->class[nClassId]->numstyles;
    msMaybeAllocateClassStyle(psLayer->class[nClassId], iStyle);

    msSLDParsePolygonFill(psFill, psLayer->class[nClassId]->styles[iStyle],
                          psLayer->map);

    if (nOffsetX > 0 && nOffsetY > 0) {
      psLayer->class[nClassId]->styles[iStyle]->offsetx = nOffsetX;
      psLayer->class[nClassId]->styles[iStyle]->offsety = nOffsetY;
    }
  }
  /* stroke wich corresponds to the outilne in mapserver */
  /* is drawn after the fill */
  psStroke =  CPLGetXMLNode(psRoot, "Stroke");
  if (psStroke) {
    /* -------------------------------------------------------------------- */
    /*      there was a fill so add a style to the last class created       */
    /*      by the fill                                                     */
    /* -------------------------------------------------------------------- */
    if (psFill && psLayer->numclasses > 0) {
      nClassId =psLayer->numclasses-1;
      iStyle = psLayer->class[nClassId]->numstyles;
      msMaybeAllocateClassStyle(psLayer->class[nClassId], iStyle);
    } else {
      if (bNewClass || psLayer->numclasses <= 0) {
        if (msGrowLayerClasses(psLayer) == NULL)
          return MS_FAILURE;
        initClass(psLayer->class[psLayer->numclasses]);
        nClassId = psLayer->numclasses;
        psLayer->numclasses++;
      } else
        nClassId = psLayer->numclasses-1;

      iStyle = psLayer->class[nClassId]->numstyles;
      msMaybeAllocateClassStyle(psLayer->class[nClassId], iStyle);

    }
    msSLDParseStroke(psStroke, psLayer->class[nClassId]->styles[iStyle],
                     psLayer->map, 1);

    if (nOffsetX > 0 && nOffsetY > 0) {
      psLayer->class[nClassId]->styles[iStyle]->offsetx = nOffsetX;
      psLayer->class[nClassId]->styles[iStyle]->offsety = nOffsetY;
    }
  }

  return MS_SUCCESS;
}


/************************************************************************/
/*    void msSLDParsePolygonFill(CPLXMLNode *psFill, styleObj *psStyle, */
/*                                 mapObj *map)                         */
/*                                                                      */
/*      Parse the Fill node for a polygon into a style.                 */
/************************************************************************/
int msSLDParsePolygonFill(CPLXMLNode *psFill, styleObj *psStyle,
                          mapObj *map)
{
  CPLXMLNode *psCssParam, *psGraphicFill;
  char *psColor=NULL, *psFillName=NULL;
  int nLength = 0;

  if (!psFill || !psStyle || !map)
    return MS_FAILURE;

  /* sets the default fill color defined in the spec #808080 */
  psStyle->color.red = 128;
  psStyle->color.green = 128;
  psStyle->color.blue = 128;

  psCssParam =  CPLGetXMLNode(psFill, "CssParameter");
  /*sld 1.1 used SvgParameter*/
  if (psCssParam == NULL)
    psCssParam =  CPLGetXMLNode(psFill, "SvgParameter");

  while (psCssParam && psCssParam->pszValue &&
         (strcasecmp(psCssParam->pszValue, "CssParameter") == 0 ||
          strcasecmp(psCssParam->pszValue, "SvgParameter") == 0)) {
    psFillName = (char*)CPLGetXMLValue(psCssParam, "name", NULL);
    if (psFillName) {
      if (strcasecmp(psFillName, "fill") == 0) {
        if(psCssParam->psChild && psCssParam->psChild->psNext &&
            psCssParam->psChild->psNext->pszValue)
          psColor = psCssParam->psChild->psNext->pszValue;

        if (psColor) {
          nLength = strlen(psColor);
          /* expecting hexadecimal ex : #aaaaff */
          if (nLength == 7 && psColor[0] == '#') {
            psStyle->color.red = msHexToInt(psColor+1);
            psStyle->color.green = msHexToInt(psColor+3);
            psStyle->color.blue= msHexToInt(psColor+5);
          }
        }
      } else if (strcasecmp(psFillName, "fill-opacity") == 0) {
        if(psCssParam->psChild &&  psCssParam->psChild->psNext &&
            psCssParam->psChild->psNext->pszValue) {
          psStyle->color.alpha = (int)(atof(psCssParam->psChild->psNext->pszValue)*255);
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


  return MS_SUCCESS;
}


/************************************************************************/
/*                      msSLDParseGraphicFillOrStroke                   */
/*                                                                      */
/*      Parse the GraphicFill Or GraphicStroke node : look for a        */
/*      Marker symbol and set the style for that symbol.                */
/************************************************************************/
int msSLDParseGraphicFillOrStroke(CPLXMLNode *psRoot,
                                  char *pszDashValue,
                                  styleObj *psStyle, mapObj *map,
                                  int bPointLayer)
{
  CPLXMLNode  *psCssParam, *psGraphic, *psExternalGraphic, *psMark, *psSize;
  CPLXMLNode *psWellKnownName, *psStroke, *psFill;
  CPLXMLNode *psDisplacement=NULL, *psDisplacementX=NULL, *psDisplacementY=NULL;
  CPLXMLNode *psOpacity=NULL, *psRotation=NULL;
  char *psName=NULL, *psValue = NULL;
  int nLength = 0;
  char *pszSymbolName = NULL;
  int bFilled = 0;
  CPLXMLNode *psPropertyName=NULL;
  char szTmp[256];

  bPointLayer=0;

  if (!psRoot || !psStyle || !map)
    return MS_FAILURE;
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
  if (psGraphic) {
    /* extract symbol size */
    psSize = CPLGetXMLNode(psGraphic, "Size");
    if (psSize)
      psStyle->size = _msSLDParseSizeParameter(psSize);
    else {
      /*do not set a default for external symbols #2305*/
      psExternalGraphic =  CPLGetXMLNode(psGraphic, "ExternalGraphic");
      if (!psExternalGraphic)
        psStyle->size = 6; /* default value */
    }

    /*SLD 1.1.0 extract opacity, rotation, displacement*/
    psOpacity = CPLGetXMLNode(psGraphic, "Opacity");
    if (psOpacity && psOpacity->psChild && psOpacity->psChild->pszValue)
      psStyle->opacity = (int)(atof(psOpacity->psChild->pszValue) * 100);

    psRotation = CPLGetXMLNode(psGraphic, "Rotation");
    if (psRotation) {
      psPropertyName = CPLGetXMLNode(psRotation, "PropertyName");
      if (psPropertyName) {
        snprintf(szTmp, sizeof(szTmp), "%s", CPLGetXMLValue(psPropertyName, NULL, NULL));
        psStyle->bindings[MS_STYLE_BINDING_ANGLE].item = msStrdup(szTmp);
        psStyle->numbindings++;
      } else {
        if (psRotation->psChild && psRotation->psChild->pszValue)
          psStyle->angle = atof(psRotation->psChild->pszValue);
      }
    }
    psDisplacement = CPLGetXMLNode(psGraphic, "Displacement");
    if (psDisplacement) {
      psDisplacementX = CPLGetXMLNode(psDisplacement, "DisplacementX");
      psDisplacementY = CPLGetXMLNode(psDisplacement, "DisplacementY");
      /* psCssParam->psChild->psNext->pszValue) */
      if (psDisplacementX &&
          psDisplacementX->psChild &&
          psDisplacementX->psChild->pszValue &&
          psDisplacementY &&
          psDisplacementY->psChild &&
          psDisplacementY->psChild->pszValue) {
        psStyle->offsetx = atoi(psDisplacementX->psChild->pszValue);
        psStyle->offsety = atoi(psDisplacementY->psChild->pszValue);
      }
    }
    /* extract symbol */
    psMark =  CPLGetXMLNode(psGraphic, "Mark");
    if (psMark) {
      pszSymbolName = NULL;
      psWellKnownName =  CPLGetXMLNode(psMark, "WellKnownName");
      if (psWellKnownName && psWellKnownName->psChild &&
          psWellKnownName->psChild->pszValue)
        pszSymbolName =
          msStrdup(psWellKnownName->psChild->pszValue);

      /* default symbol is square */

      if (!pszSymbolName || !*pszSymbolName || 
          (strcasecmp(pszSymbolName, "square") != 0 &&
           strcasecmp(pszSymbolName, "circle") != 0 &&
           strcasecmp(pszSymbolName, "triangle") != 0 &&
           strcasecmp(pszSymbolName, "star") != 0 &&
           strcasecmp(pszSymbolName, "cross") != 0 &&
           strcasecmp(pszSymbolName, "x") != 0)) {
        if (!pszSymbolName || !*pszSymbolName || msGetSymbolIndex(&map->symbolset, pszSymbolName,  MS_FALSE) < 0) {
          msFree(pszSymbolName);
          pszSymbolName = msStrdup("square");
        }
      }


      /* check if the symbol should be filled or not */
      psFill = CPLGetXMLNode(psMark, "Fill");
      psStroke = CPLGetXMLNode(psMark, "Stroke");

      if (psFill || psStroke) {
        if (psFill)
          bFilled = 1;
        else
          bFilled = 0;

        if (psFill) {
          psCssParam =  CPLGetXMLNode(psFill, "CssParameter");
          /*sld 1.1 used SvgParameter*/
          if (psCssParam == NULL)
            psCssParam =  CPLGetXMLNode(psFill, "SvgParameter");

          while (psCssParam && psCssParam->pszValue &&
                 (strcasecmp(psCssParam->pszValue, "CssParameter") == 0 ||
                  strcasecmp(psCssParam->pszValue, "SvgParameter") == 0)) {
            psName =
              (char*)CPLGetXMLValue(psCssParam, "name", NULL);
            if (psName &&
                strcasecmp(psName, "fill") == 0) {
              if(psCssParam->psChild &&
                  psCssParam->psChild->psNext &&
                  psCssParam->psChild->psNext->pszValue)
                psValue = psCssParam->psChild->psNext->pszValue;

              if (psValue) {
                nLength = strlen(psValue);
                if (nLength == 7 && psValue[0] == '#') {
                  msSLDSetColorObject(psValue,
                                      &psStyle->color);
                }
              }
            } else if (psName &&
                       strcasecmp(psName, "fill-opacity") == 0) {
              if(psCssParam->psChild &&
                  psCssParam->psChild->psNext &&
                  psCssParam->psChild->psNext->pszValue)
                psValue = psCssParam->psChild->psNext->pszValue;

              if (psValue) {
                psStyle->color.alpha = (int)(atof(psValue)*255);
              }
            }

            psCssParam = psCssParam->psNext;
          }
        }
        if (psStroke) {
          psCssParam =  CPLGetXMLNode(psStroke, "CssParameter");
          /*sld 1.1 used SvgParameter*/
          if (psCssParam == NULL)
            psCssParam =  CPLGetXMLNode(psStroke, "SvgParameter");

          while (psCssParam && psCssParam->pszValue &&
                 (strcasecmp(psCssParam->pszValue, "CssParameter") == 0 ||
                  strcasecmp(psCssParam->pszValue, "SvgParameter") == 0)) {
            psName =
              (char*)CPLGetXMLValue(psCssParam, "name", NULL);
            if (psName &&
                strcasecmp(psName, "stroke") == 0) {
              if(psCssParam->psChild &&
                  psCssParam->psChild->psNext &&
                  psCssParam->psChild->psNext->pszValue)
                psValue = psCssParam->psChild->psNext->pszValue;

              if (psValue) {
                nLength = strlen(psValue);
                if (nLength == 7 && psValue[0] == '#') {
                  msSLDSetColorObject(psValue,
                                      &psStyle->outlinecolor);
                }
              }
            } else if (psName &&
                       strcasecmp(psName, "stroke-opacity") == 0) {
              if(psCssParam->psChild &&
                  psCssParam->psChild->psNext &&
                  psCssParam->psChild->psNext->pszValue)
                psValue = psCssParam->psChild->psNext->pszValue;

              if (psValue) {
                psStyle->outlinecolor.alpha = (int)(atof(psValue)*255);
              }
            } else if (psName &&
                       strcasecmp(psName, "stroke-width") == 0) {
              if(psCssParam->psChild &&
                  psCssParam->psChild->psNext &&
                  psCssParam->psChild->psNext->pszValue)
                psValue = psCssParam->psChild->psNext->pszValue;

              if (psValue) {
                psStyle->width = atof(psValue);
              }
            }

            psCssParam = psCssParam->psNext;
          }
        }

      }
      /* set the default color if color is not not already set */
      if ((psStyle->color.red < 0 ||
           psStyle->color.green == -1 ||
           psStyle->color.blue == -1) &&
          (psStyle->outlinecolor.red == -1 ||
           psStyle->outlinecolor.green == -1 ||
           psStyle->outlinecolor.blue == -1)) {
        psStyle->color.red = 128;
        psStyle->color.green = 128;
        psStyle->color.blue = 128;
      }


      /* Get the corresponding symbol id  */
      psStyle->symbol = msSLDGetMarkSymbol(map, pszSymbolName, bFilled);
      if (psStyle->symbol > 0 &&
          psStyle->symbol < map->symbolset.numsymbols)
        psStyle->symbolname =
          msStrdup(map->symbolset.symbol[psStyle->symbol]->name);

    } else {
      psExternalGraphic =  CPLGetXMLNode(psGraphic, "ExternalGraphic");
      if (psExternalGraphic)
        msSLDParseExternalGraphic(psExternalGraphic, psStyle, map);
    }
    msFree(pszSymbolName);
  }

  return MS_SUCCESS;
}


/************************************************************************/
/*                            msSLDGetMarkSymbol                        */
/*                                                                      */
/*      Get a Mark symbol using the name. Mark symbols can be           */
/*      square, circle, triangle, star, cross, x.                       */
/*      If the symbol does not exsist add it to the symbol list.        */
/************************************************************************/
int msSLDGetMarkSymbol(mapObj *map, char *pszSymbolName, int bFilled)
{
  int nSymbolId = 0;
  symbolObj *psSymbol = NULL;

  if (!map || !pszSymbolName)
    return 0;

  if (strcasecmp(pszSymbolName, "square") == 0) {
    if (bFilled)
      nSymbolId = msGetSymbolIndex(&map->symbolset,
                                   SLD_MARK_SYMBOL_SQUARE_FILLED,
                                   MS_FALSE);
    else
      nSymbolId = msGetSymbolIndex(&map->symbolset,
                                   SLD_MARK_SYMBOL_SQUARE,
                                   MS_FALSE);
  } else if (strcasecmp(pszSymbolName, "circle") == 0) {

    if (bFilled)
      nSymbolId = msGetSymbolIndex(&map->symbolset,
                                   SLD_MARK_SYMBOL_CIRCLE_FILLED,
                                   MS_FALSE);
    else
      nSymbolId = msGetSymbolIndex(&map->symbolset,
                                   SLD_MARK_SYMBOL_CIRCLE,
                                   MS_FALSE);
  } else if (strcasecmp(pszSymbolName, "triangle") == 0) {

    if (bFilled)
      nSymbolId = msGetSymbolIndex(&map->symbolset,
                                   SLD_MARK_SYMBOL_TRIANGLE_FILLED,
                                   MS_FALSE);
    else
      nSymbolId = msGetSymbolIndex(&map->symbolset,
                                   SLD_MARK_SYMBOL_TRIANGLE,
                                   MS_FALSE);
  } else if (strcasecmp(pszSymbolName, "star") == 0) {

    if (bFilled)
      nSymbolId = msGetSymbolIndex(&map->symbolset,
                                   SLD_MARK_SYMBOL_STAR_FILLED,
                                   MS_FALSE);
    else
      nSymbolId = msGetSymbolIndex(&map->symbolset,
                                   SLD_MARK_SYMBOL_STAR,
                                   MS_FALSE);
  } else if (strcasecmp(pszSymbolName, "cross") == 0) {

    if (bFilled)
      nSymbolId = msGetSymbolIndex(&map->symbolset,
                                   SLD_MARK_SYMBOL_CROSS_FILLED,
                                   MS_FALSE);
    else
      nSymbolId = msGetSymbolIndex(&map->symbolset,
                                   SLD_MARK_SYMBOL_CROSS,
                                   MS_FALSE);
  } else if (strcasecmp(pszSymbolName, "x") == 0) {

    if (bFilled)
      nSymbolId = msGetSymbolIndex(&map->symbolset,
                                   SLD_MARK_SYMBOL_X_FILLED,
                                   MS_FALSE);
    else
      nSymbolId = msGetSymbolIndex(&map->symbolset,
                                   SLD_MARK_SYMBOL_X,
                                   MS_FALSE);
  } else {
    nSymbolId = msGetSymbolIndex(&map->symbolset,
                                 pszSymbolName,
                                 MS_FALSE);
  }

  if (nSymbolId <= 0) {
    if( (psSymbol = msGrowSymbolSet(&(map->symbolset))) == NULL)
      return 0; /* returns 0 for no symbol */

    nSymbolId = map->symbolset.numsymbols;
    map->symbolset.numsymbols++;
    initSymbol(psSymbol);
    psSymbol->inmapfile = MS_TRUE;
    psSymbol->sizex = 1;
    psSymbol->sizey = 1;

    if (strcasecmp(pszSymbolName, "square") == 0) {
      if (bFilled)
        psSymbol->name = msStrdup(SLD_MARK_SYMBOL_SQUARE_FILLED);
      else
        psSymbol->name = msStrdup(SLD_MARK_SYMBOL_SQUARE);

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
    } else if (strcasecmp(pszSymbolName, "circle") == 0) {
      if (bFilled)
        psSymbol->name = msStrdup(SLD_MARK_SYMBOL_CIRCLE_FILLED);
      else
        psSymbol->name = msStrdup(SLD_MARK_SYMBOL_CIRCLE);

      psSymbol->type = MS_SYMBOL_ELLIPSE;
      if (bFilled)
        psSymbol->filled = MS_TRUE;

      psSymbol->points[psSymbol->numpoints].x = 1;
      psSymbol->points[psSymbol->numpoints].y = 1;
      psSymbol->sizex = 1;
      psSymbol->sizey = 1;
      psSymbol->numpoints++;
    } else if (strcasecmp(pszSymbolName, "triangle") == 0) {
      if (bFilled)
        psSymbol->name = msStrdup(SLD_MARK_SYMBOL_TRIANGLE_FILLED);
      else
        psSymbol->name = msStrdup(SLD_MARK_SYMBOL_TRIANGLE);

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

    } else if (strcasecmp(pszSymbolName, "star") == 0) {
      if (bFilled)
        psSymbol->name = msStrdup(SLD_MARK_SYMBOL_STAR_FILLED);
      else
        psSymbol->name = msStrdup(SLD_MARK_SYMBOL_STAR);

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
    else if (strcasecmp(pszSymbolName, "cross") == 0) {
      /* NEVER FILL CROSS */
      /* if (bFilled) */
      /* psSymbol->name = msStrdup(SLD_MARK_SYMBOL_CROSS_FILLED); */
      /* else */
      psSymbol->name = msStrdup(SLD_MARK_SYMBOL_CROSS);

      psSymbol->type = MS_SYMBOL_VECTOR;
      /* if (bFilled) */
      /* psSymbol->filled = MS_TRUE; */

      psSymbol->points[psSymbol->numpoints].x = 0.5;
      psSymbol->points[psSymbol->numpoints].y = 0;
      psSymbol->numpoints++;
      psSymbol->points[psSymbol->numpoints].x = 0.5;
      psSymbol->points[psSymbol->numpoints].y = 1;
      psSymbol->numpoints++;
      psSymbol->points[psSymbol->numpoints].x = -99;
      psSymbol->points[psSymbol->numpoints].y = -99;
      psSymbol->numpoints++;
      psSymbol->points[psSymbol->numpoints].x = 0;
      psSymbol->points[psSymbol->numpoints].y = 0.5;
      psSymbol->numpoints++;
      psSymbol->points[psSymbol->numpoints].x = 1;
      psSymbol->points[psSymbol->numpoints].y = 0.5;
      psSymbol->numpoints++;
    } else if (strcasecmp(pszSymbolName, "x") == 0) {
      /* NEVER FILL X */
      /* if (bFilled) */
      /* psSymbol->name = msStrdup(SLD_MARK_SYMBOL_X_FILLED); */
      /* else */
      psSymbol->name = msStrdup(SLD_MARK_SYMBOL_X);

      psSymbol->type = MS_SYMBOL_VECTOR;
      /* if (bFilled) */
      /* psSymbol->filled = MS_TRUE; */
      psSymbol->points[psSymbol->numpoints].x = 0;
      psSymbol->points[psSymbol->numpoints].y = 0;
      psSymbol->numpoints++;
      psSymbol->points[psSymbol->numpoints].x = 1;
      psSymbol->points[psSymbol->numpoints].y = 1;
      psSymbol->numpoints++;
      psSymbol->points[psSymbol->numpoints].x = -99;
      psSymbol->points[psSymbol->numpoints].y = -99;
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

static const unsigned char PNGsig[8] = {137, 80, 78, 71, 13, 10, 26, 10}; /* 89 50 4E 47 0D 0A 1A 0A hex */

/************************************************************************/
/*                          msSLDGetGraphicSymbol                       */
/*                                                                      */
/*      Create a symbol entry for an inmap pixmap symbol. Returns       */
/*      the symbol id.                                                  */
/************************************************************************/
int msSLDGetGraphicSymbol(mapObj *map, char *pszFileName,  char* extGraphicName,
                          int nGap)
{
  int nSymbolId = 0;
  symbolObj *psSymbol = NULL;


  if (map && pszFileName) {
    if( (psSymbol = msGrowSymbolSet(&(map->symbolset))) == NULL)
      return 0; /* returns 0 for no symbol */
    nSymbolId = map->symbolset.numsymbols;
    map->symbolset.numsymbols++;
    initSymbol(psSymbol);
    psSymbol->inmapfile = MS_TRUE;
    psSymbol->type = MS_SYMBOL_PIXMAP;
    psSymbol->name = msStrdup(extGraphicName);
    psSymbol->imagepath = msStrdup(pszFileName);
    psSymbol->full_pixmap_path = msStrdup(pszFileName);
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
int msSLDParsePointSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer,
                              int bNewClass)
{
  int nClassId = 0;
  int iStyle = 0;

  if (!psRoot || !psLayer)
    return MS_FAILURE;

  if (bNewClass || psLayer->numclasses <= 0) {
    if (msGrowLayerClasses(psLayer) == NULL)
      return MS_FAILURE;
    initClass(psLayer->class[psLayer->numclasses]);
    nClassId = psLayer->numclasses;
    psLayer->numclasses++;
  } else
    nClassId = psLayer->numclasses-1;

  iStyle = psLayer->class[nClassId]->numstyles;
  msMaybeAllocateClassStyle(psLayer->class[nClassId], iStyle);


  msSLDParseGraphicFillOrStroke(psRoot, NULL,
                                psLayer->class[nClassId]->styles[iStyle],
                                psLayer->map, 1);

  return MS_SUCCESS;
}


/************************************************************************/
/*                        msSLDParseExternalGraphic                     */
/*                                                                      */
/*      Parse extrenal graphic node : download the symbol referneced    */
/*      by the URL and create a PIXMAP inmap symbol. Only GIF and       */
/*      PNG are supported.                                              */
/************************************************************************/
int msSLDParseExternalGraphic(CPLXMLNode *psExternalGraphic,
                              styleObj *psStyle,  mapObj *map)
{
  /* needed for libcurl function msHTTPGetFile in maphttp.c */
#if defined(USE_CURL)

  char *pszFormat = NULL;
  CPLXMLNode *psURL=NULL, *psFormat=NULL, *psTmp=NULL;
  char *pszURL=NULL;

  if (!psExternalGraphic || !psStyle || !map)
    return MS_FAILURE;

  psFormat = CPLGetXMLNode(psExternalGraphic, "Format");
  if (psFormat && psFormat->psChild && psFormat->psChild->pszValue)
    pszFormat = psFormat->psChild->pszValue;

  /* supports GIF and PNG */
  if (pszFormat &&
      (strcasecmp(pszFormat, "GIF") == 0 ||
       strcasecmp(pszFormat, "image/gif") == 0 ||
       strcasecmp(pszFormat, "PNG") == 0 ||
       strcasecmp(pszFormat, "image/png") == 0)) {

    /* <OnlineResource xmlns:xlink="http://www.w3.org/1999/xlink" xlink:type="simple" xlink:href="http://www.vendor.com/geosym/2267.svg"/> */
    psURL = CPLGetXMLNode(psExternalGraphic, "OnlineResource");
    if (psURL && psURL->psChild) {
      psTmp =  psURL->psChild;
      while (psTmp != NULL &&
             psTmp->pszValue &&
             strcasecmp(psTmp->pszValue, "xlink:href") != 0) {
        psTmp = psTmp->psNext;
      }
      if (psTmp && psTmp->psChild) {
        pszURL = (char*)psTmp->psChild->pszValue;

        /*external symbols using http will be automaticallly downloaded. The file should be
          saved in a temporary directory (msAddImageSymbol) #2305*/
        psStyle->symbol = msGetSymbolIndex(&map->symbolset,
                                           pszURL,
                                           MS_TRUE);

        if (psStyle->symbol > 0 && psStyle->symbol < map->symbolset.numsymbols)
          psStyle->symbolname = msStrdup(map->symbolset.symbol[psStyle->symbol]->name);

        /* set the color parameter if not set. Does not make sense */
        /* for pixmap but mapserver needs it. */
        if (psStyle->color.red == -1 || psStyle->color.green || psStyle->color.blue) {
          psStyle->color.red = 0;
          psStyle->color.green = 0;
          psStyle->color.blue = 0;
        }
      }
    }
  }

  return MS_SUCCESS;
#else
  return MS_FAILURE;
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
int msSLDParseTextSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer,
                             int bOtherSymboliser)
{
  int nStyleId=0, nClassId=0;

  if (!psRoot || !psLayer)
    return MS_FAILURE;

  if (!bOtherSymboliser) {
    if (msGrowLayerClasses(psLayer) == NULL)
      return MS_FAILURE;
    initClass(psLayer->class[psLayer->numclasses]);
    nClassId = psLayer->numclasses;
    psLayer->numclasses++;
    msMaybeAllocateClassStyle(psLayer->class[nClassId], 0);
    nStyleId = 0;
  } else {
    nClassId = psLayer->numclasses - 1;
    if (nClassId >= 0)/* should always be true */
      nStyleId = psLayer->class[nClassId]->numstyles -1;
  }

  if (nStyleId >= 0 && nClassId >= 0) /* should always be true */
    msSLDParseTextParams(psRoot, psLayer,
                         psLayer->class[nClassId]);

  return MS_SUCCESS;
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
/*                                                                      */
/*      SLD 1.1                                                         */
/*                                                                      */
/*      <xsd:element name="RasterSymbolizer" type="se:RasterSymbolizerType" substitutionGroup="se:Symbolizer"/>*/
/*      <xsd:complexType name="RasterSymbolizerType">                   */
/*      <xsd:complexContent>                                            */
/*      <xsd:extension base="se:SymbolizerType">                        */
/*      <xsd:sequence>                                                  */
/*      <xsd:element ref="se:Geometry" minOccurs="0"/>                  */
/*      <xsd:element ref="se:Opacity" minOccurs="0"/>                   */
/*      <xsd:element ref="se:ChannelSelection" minOccurs="0"/>          */
/*      <xsd:element ref="se:OverlapBehavior" minOccurs="0"/>           */
/*      <xsd:element ref="se:ColorMap" minOccurs="0"/>                  */
/*      <xsd:element ref="se:ContrastEnhancement" minOccurs="0"/>       */
/*      <xsd:element ref="se:ShadedRelief" minOccurs="0"/>              */
/*      <xsd:element ref="se:ImageOutline" minOccurs="0"/>              */
/*      </xsd:sequence>                                                 */
/*      </xsd:extension>                                                */
/*      </xsd:complexContent>                                           */
/*      </xsd:complexType>                                              */
/*                                                                      */
/*      <xsd:element name="ColorMap" type="se:ColorMapType"/>           */
/*      <xsd:complexType name="ColorMapType">                           */
/*      <xsd:choice>                                                    */
/*      <xsd:element ref="se:Categorize"/>                              */
/*      <xsd:element ref="se:Interpolate"/>                             */
/*      </xsd:choice>                                                   */
/*      </xsd:complexType>                                              */
/*                                                                      */
/*      <xsd:element name="Categorize" type="se:CategorizeType" substitutionGroup="se:Function"/>*/
/*      <xsd:complexType name="CategorizeType">                         */
/*      <xsd:complexContent>                                            */
/*      <xsd:extension base="se:FunctionType">                          */
/*      <xsd:sequence>                                                  */
/*      <xsd:element ref="se:LookupValue"/>                             */
/*      <xsd:element ref="se:Value"/>                                   */
/*      <xsd:sequence minOccurs="0" maxOccurs="unbounded">              */
/*      <xsd:element ref="se:Threshold"/>                               */
/*      <xsd:element ref="se:Value"/>                                   */
/*      </xsd:sequence>                                                 */
/*      </xsd:sequence>                                                 */
/*      <xsd:attribute name="threshholdsBelongTo" type="se:ThreshholdsBelongToType" use="optional"/>*/
/*      </xsd:extension>                                                */
/*      </xsd:complexContent>                                           */
/*      </xsd:complexType>                                              */
/*      <xsd:element name="LookupValue" type="se:ParameterValueType"/>  */
/*      <xsd:element name="Value" type=" se:ParameterValueType"/>       */
/*      <xsd:element name="Threshold" type=" se:ParameterValueType"/>   */
/*      <xsd:simpleType name="ThreshholdsBelongToType">                 */
/*      <xsd:restriction base="xsd:token">                              */
/*      <xsd:enumeration value="succeeding"/>                           */
/*      <xsd:enumeration value="preceding"/>                            */
/*      </xsd:restriction>                                              */
/*      </xsd:simpleType>                                               */
/*                                                                      */
/************************************************************************/
int msSLDParseRasterSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer)
{
  CPLXMLNode  *psColorMap = NULL, *psColorEntry = NULL, *psOpacity=NULL;
  char *pszColor=NULL, *pszQuantity=NULL;
  char *pszPreviousColor=NULL, *pszPreviousQuality=NULL;
  colorObj sColor;
  char szExpression[100];
  int nClassId = 0;
  double dfOpacity = 1.0;
  char *pszLabel = NULL,  *pszPreviousLabel = NULL;
  char *pch = NULL, *pchPrevious=NULL;

  CPLXMLNode  *psNode=NULL, *psCategorize=NULL;
  char *pszTmp = NULL;
  int nValues=0, nThresholds=0;
  int i,nMaxValues= 100, nMaxThreshold=100;

  if (!psRoot || !psLayer)
    return MS_FAILURE;

  /* ==================================================================== */
  /*      The default opacity value is 0 : we set it here to -1           */
  /*      so that when testing the values in msSLDApplySLD (to be         */
  /*      applied on the layer), we can assume that a value of 0 comes    */
  /*      from the sld.                                                   */
  /* ==================================================================== */
  psLayer->opacity = -1;

  psOpacity = CPLGetXMLNode(psRoot, "Opacity");
  if (psOpacity) {
    if (psOpacity->psChild && psOpacity->psChild->pszValue)
      dfOpacity = atof(psOpacity->psChild->pszValue);

    /* values in sld goes from 0.0 (for transparent) to 1.0 (for full opacity); */
    if (dfOpacity >=0.0 && dfOpacity <=1.0)
      psLayer->opacity = (int)(dfOpacity * 100);
    else {
      msSetError(MS_WMSERR, "Invalid opacity value. Values should be between 0.0 and 1.0", "msSLDParseRasterSymbolizer()");
      return MS_FAILURE;
    }
  }
  psColorMap = CPLGetXMLNode(psRoot, "ColorMap");
  if (psColorMap) {
    psColorEntry = CPLGetXMLNode(psColorMap, "ColorMapEntry");

    if (psColorEntry) { /*SLD 1.0*/
      while (psColorEntry && psColorEntry->pszValue &&
             strcasecmp(psColorEntry->pszValue, "ColorMapEntry") == 0) {
        pszColor = (char *)CPLGetXMLValue(psColorEntry, "color", NULL);
        pszQuantity = (char *)CPLGetXMLValue(psColorEntry, "quantity", NULL);
        pszLabel = (char *)CPLGetXMLValue(psColorEntry, "label", NULL);

        if (pszColor && pszQuantity) {
          if (pszPreviousColor && pszPreviousQuality) {
            if (strlen(pszPreviousColor) == 7 &&
                pszPreviousColor[0] == '#' &&
                strlen(pszColor) == 7 && pszColor[0] == '#') {
              sColor.red = msHexToInt(pszPreviousColor+1);
              sColor.green= msHexToInt(pszPreviousColor+3);
              sColor.blue = msHexToInt(pszPreviousColor+5);

              /* pszQuantity and pszPreviousQuality may be integer or float */
              pchPrevious=strchr(pszPreviousQuality,'.');
              pch=strchr(pszQuantity,'.');
              if (pchPrevious==NULL && pch==NULL) {
                snprintf(szExpression, sizeof(szExpression),
                         "([pixel] >= %d AND [pixel] < %d)",
                         atoi(pszPreviousQuality),
                         atoi(pszQuantity));
              } else if (pchPrevious != NULL && pch==NULL) {
                snprintf(szExpression, sizeof(szExpression),
                         "([pixel] >= %f AND [pixel] < %d)",
                         atof(pszPreviousQuality),
                         atoi(pszQuantity));
              } else if (pchPrevious == NULL && pch != NULL) {
                snprintf(szExpression, sizeof(szExpression),
                         "([pixel] >= %d AND [pixel] < %f)",
                         atoi(pszPreviousQuality),
                         atof(pszQuantity));
              } else {
                snprintf(szExpression, sizeof(szExpression),
                         "([pixel] >= %f AND [pixel] < %f)",
                         atof(pszPreviousQuality),
                         atof(pszQuantity));
              }


              if (msGrowLayerClasses(psLayer) == NULL)
                return MS_FAILURE;
              else {
                initClass(psLayer->class[psLayer->numclasses]);
                psLayer->numclasses++;
                nClassId = psLayer->numclasses-1;

                /*set the class name using the label. If label not defined
                  set it with the quantity*/
                if (pszPreviousLabel)
                  psLayer->class[nClassId]->name = msStrdup(pszPreviousLabel);
                else
                  psLayer->class[nClassId]->name = msStrdup(pszPreviousQuality);

                msMaybeAllocateClassStyle(psLayer->class[nClassId], 0);

                psLayer->class[nClassId]->styles[0]->color.red =
                      sColor.red;
              psLayer->class[nClassId]->styles[0]->color.green =
                      sColor.green;
              psLayer->class[nClassId]->styles[0]->color.blue =
                      sColor.blue;

                if (psLayer->classitem &&
                    strcasecmp(psLayer->classitem, "[pixel]") != 0)
                  free(psLayer->classitem);
                psLayer->classitem = msStrdup("[pixel]");

                msLoadExpressionString(&psLayer->class[nClassId]->expression,
                                       szExpression);


              }
            } else {
              msSetError(MS_WMSERR,
                         "Invalid ColorMap Entry.",
                         "msSLDParseRasterSymbolizer()");
              return MS_FAILURE;
            }

          }

          pszPreviousColor = pszColor;
          pszPreviousQuality = pszQuantity;
          pszPreviousLabel = pszLabel;

        }
        psColorEntry = psColorEntry->psNext;
      }
      /* do the last Color Map Entry */
      if (pszColor && pszQuantity) {
        if (strlen(pszColor) == 7 && pszColor[0] == '#') {
          sColor.red = msHexToInt(pszColor+1);
          sColor.green= msHexToInt(pszColor+3);
          sColor.blue = msHexToInt(pszColor+5);

          /* pszQuantity may be integer or float */
          pch=strchr(pszQuantity,'.');
          if (pch==NULL) {
            snprintf(szExpression, sizeof(szExpression), "([pixel] = %d)", atoi(pszQuantity));
          } else {
            snprintf(szExpression, sizeof(szExpression), "([pixel] = %f)", atof(pszQuantity));
          }

          if (msGrowLayerClasses(psLayer) == NULL)
            return MS_FAILURE;
          else {
            initClass(psLayer->class[psLayer->numclasses]);
            psLayer->numclasses++;
            nClassId = psLayer->numclasses-1;
            msMaybeAllocateClassStyle(psLayer->class[nClassId], 0);
            if (pszLabel)
              psLayer->class[nClassId]->name = msStrdup(pszLabel);
            else
              psLayer->class[nClassId]->name = msStrdup(pszQuantity);
            psLayer->class[nClassId]->numstyles = 1;
            psLayer->class[nClassId]->styles[0]->color.red =
                  sColor.red;
          psLayer->class[nClassId]->styles[0]->color.green =
                  sColor.green;
          psLayer->class[nClassId]->styles[0]->color.blue =
                  sColor.blue;

            if (psLayer->classitem &&
                strcasecmp(psLayer->classitem, "[pixel]") != 0)
              free(psLayer->classitem);
            psLayer->classitem = msStrdup("[pixel]");

            msLoadExpressionString(&psLayer->class[nClassId]->expression,
                                   szExpression);
          }
        }
      }
    } else if ((psCategorize = CPLGetXMLNode(psColorMap, "Categorize"))) {
      char** papszValues = (char **)malloc(sizeof(char*)*nMaxValues);
      char** papszThresholds = (char **)malloc(sizeof(char*)*nMaxThreshold);
      psNode =  CPLGetXMLNode(psCategorize, "Value");
      while (psNode && psNode->pszValue &&
             psNode->psChild && psNode->psChild->pszValue)

      {
        if (strcasecmp(psNode->pszValue, "Value") == 0) {
          papszValues[nValues] =  psNode->psChild->pszValue;
          nValues++;
          if (nValues == nMaxValues) {
            nMaxValues +=100;
            papszValues = (char **)realloc(papszValues, sizeof(char*)*nMaxValues);
          }
        } else if (strcasecmp(psNode->pszValue, "Threshold") == 0) {
          papszThresholds[nThresholds] =  psNode->psChild->pszValue;
          nThresholds++;
          if (nValues == nMaxThreshold) {
            nMaxThreshold += 100;
            papszThresholds = (char **)realloc(papszThresholds, sizeof(char*)*nMaxThreshold);
          }
        }
        psNode = psNode->psNext;
      }

      if (nValues == nThresholds+1) {
        /*free existing classes*/
        for(i=0; i<psLayer->numclasses; i++) {
          if (psLayer->class[i] != NULL) {
            psLayer->class[i]->layer=NULL;
            if ( freeClass(psLayer->class[i]) == MS_SUCCESS ) {
              msFree(psLayer->class[i]);
              psLayer->class[i]=NULL;
            }
          }
        }
        psLayer->numclasses=0;
        for (i=0; i<nValues; i++) {
          pszTmp = (papszValues[i]);
          if (pszTmp && strlen(pszTmp) == 7 && pszTmp[0] == '#') {
            sColor.red = msHexToInt(pszTmp+1);
            sColor.green= msHexToInt(pszTmp+3);
            sColor.blue = msHexToInt(pszTmp+5);
            if (i == 0) {
              if (strchr(papszThresholds[i],'.'))
                snprintf(szExpression, sizeof(szExpression), "([pixel] < %f)", atof(papszThresholds[i]));
              else
                snprintf(szExpression, sizeof(szExpression), "([pixel] < %d)", atoi(papszThresholds[i]));

            } else if (i < nValues-1) {
              if (strchr(papszThresholds[i],'.'))
                snprintf(szExpression,  sizeof(szExpression),
                         "([pixel] >= %f AND [pixel] < %f)",
                         atof(papszThresholds[i-1]),
                         atof(papszThresholds[i]));
              else
                snprintf(szExpression, sizeof(szExpression),
                         "([pixel] >= %d AND [pixel] < %d)",
                         atoi(papszThresholds[i-1]),
                         atoi(papszThresholds[i]));
            } else {
              if (strchr(papszThresholds[i-1],'.'))
                snprintf(szExpression, sizeof(szExpression), "([pixel] >= %f)", atof(papszThresholds[i-1]));
              else
                snprintf(szExpression, sizeof(szExpression), "([pixel] >= %d)", atoi(papszThresholds[i-1]));
            }
            if (msGrowLayerClasses(psLayer)) {
              initClass(psLayer->class[psLayer->numclasses]);
              psLayer->numclasses++;
              nClassId = psLayer->numclasses-1;
              msMaybeAllocateClassStyle(psLayer->class[nClassId], 0);
              psLayer->class[nClassId]->numstyles = 1;
              psLayer->class[nClassId]->styles[0]->color.red =
                    sColor.red;
            psLayer->class[nClassId]->styles[0]->color.green =
                    sColor.green;
            psLayer->class[nClassId]->styles[0]->color.blue =
                    sColor.blue;
              if (psLayer->classitem &&
                  strcasecmp(psLayer->classitem, "[pixel]") != 0)
                free(psLayer->classitem);
              psLayer->classitem = msStrdup("[pixel]");
              msLoadExpressionString(&psLayer->class[nClassId]->expression,
                                     szExpression);
            }

          }
        }
      }
      free(papszValues);
      free(papszThresholds);


    } else {
      msSetError(MS_WMSERR, "Invalid SLD document. msSLDParseRaster", "");
      return MS_FAILURE;
    }
  }

  return MS_SUCCESS;
}
/************************************************************************/
/*                           msSLDParseTextParams                       */
/*                                                                      */
/*      Parse text paramaters like font, placement and color.           */
/************************************************************************/
int msSLDParseTextParams(CPLXMLNode *psRoot, layerObj *psLayer,
                         classObj *psClass)
{
  char szFontName[100];
  double  dfFontSize = 10;
  int bFontSet = 0;

  CPLXMLNode *psLabel=NULL, *psFont=NULL;
  CPLXMLNode *psCssParam = NULL;
  char *pszName=NULL, *pszFontFamily=NULL, *pszFontStyle=NULL;
  char *pszFontWeight=NULL;
  CPLXMLNode *psLabelPlacement=NULL, *psPointPlacement=NULL, *psLinePlacement=NULL;
  CPLXMLNode *psFill = NULL, *psPropertyName=NULL, *psHalo=NULL, *psHaloRadius=NULL, *psHaloFill=NULL;
  int nLength = 0;
  char *pszColor = NULL;
  /* char *pszItem = NULL; */
  CPLXMLNode *psTmpNode = NULL;
  char *pszClassText = NULL;
  char szTmp[100];
  labelObj *psLabelObj = NULL;
  szFontName[0]='\0';

  if (!psRoot || !psClass || !psLayer)
    return MS_FAILURE;

  if(psClass->numlabels == 0) {
    if(msGrowClassLabels(psClass) == NULL) return(MS_FAILURE);
    initLabel(psClass->labels[0]);
    psClass->numlabels++;
  }
  psLabelObj = psClass->labels[0];

  /*set the angle by default to auto. the angle can be
    modified Label Placement #2806*/
  psLabelObj->anglemode = MS_AUTO;


  /* label  */
  /* support literal expression  and  propertyname
   - <TextSymbolizer><Label>MY_COLUMN</Label>
   - <TextSymbolizer><Label><ogc:PropertyName>MY_COLUMN</ogc:PropertyName></Label>
  Bug 1857 */
  psLabel = CPLGetXMLNode(psRoot, "Label");
  if (psLabel ) {
    psTmpNode = psLabel->psChild;
    psPropertyName = CPLGetXMLNode(psLabel, "PropertyName");
    if (psPropertyName) {
      while (psTmpNode) {
        /* open bracket to get valid expression */
        if (pszClassText == NULL)
          pszClassText = msStringConcatenate(pszClassText, "(");

        if (psTmpNode->eType == CXT_Text && psTmpNode->pszValue) {
          pszClassText = msStringConcatenate(pszClassText, psTmpNode->pszValue);
        } else if (psTmpNode->eType == CXT_Element &&
                   strcasecmp(psTmpNode->pszValue,"PropertyName") ==0 &&
                   CPLGetXMLValue(psTmpNode, NULL, NULL)) {
          snprintf(szTmp, sizeof(szTmp), "\"[%s]\"", CPLGetXMLValue(psTmpNode, NULL, NULL));
          pszClassText = msStringConcatenate(pszClassText, szTmp);
        }
        psTmpNode = psTmpNode->psNext;

      }
      /* close bracket to get valid expression */
      if (pszClassText != NULL)
        pszClassText = msStringConcatenate(pszClassText, ")");
    } else {
      /* supports  - <TextSymbolizer><Label>MY_COLUMN</Label> */
      if (psLabel->psChild && psLabel->psChild->pszValue) {
        pszClassText = msStringConcatenate(pszClassText, "(\"[");
        pszClassText = msStringConcatenate(pszClassText, psLabel->psChild->pszValue);
        pszClassText = msStringConcatenate(pszClassText, "]\")");
      }
    }

    if (pszClassText) { /* pszItem) */

      msLoadExpressionString(&psClass->text, pszClassText);
      free(pszClassText);

      /* font */
      psFont = CPLGetXMLNode(psRoot, "Font");
      if (psFont) {
        psCssParam =  CPLGetXMLNode(psFont, "CssParameter");
        /*sld 1.1 used SvgParameter*/
        if (psCssParam == NULL)
          psCssParam =  CPLGetXMLNode(psFont, "SvgParameter");

        while (psCssParam && psCssParam->pszValue &&
               (strcasecmp(psCssParam->pszValue, "CssParameter") == 0 ||
                strcasecmp(psCssParam->pszValue, "SvgParameter") == 0)) {
          pszName = (char*)CPLGetXMLValue(psCssParam, "name", NULL);
          if (pszName) {
            if (strcasecmp(pszName, "font-family") == 0) {
              if(psCssParam->psChild && psCssParam->psChild->psNext &&
                  psCssParam->psChild->psNext->pszValue)
                pszFontFamily = psCssParam->psChild->psNext->pszValue;
            }
            /* normal, italic, oblique */
            else if (strcasecmp(pszName, "font-style") == 0) {
              if(psCssParam->psChild && psCssParam->psChild->psNext &&
                  psCssParam->psChild->psNext->pszValue)
                pszFontStyle = psCssParam->psChild->psNext->pszValue;
            }
            /* normal or bold */
            else if (strcasecmp(pszName, "font-weight") == 0) {
              if(psCssParam->psChild && psCssParam->psChild->psNext &&
                  psCssParam->psChild->psNext->pszValue)
                pszFontWeight = psCssParam->psChild->psNext->pszValue;
            }
            /* default is 10 pix */
            else if (strcasecmp(pszName, "font-size") == 0) {

              if(psCssParam->psChild && psCssParam->psChild->psNext &&
                  psCssParam->psChild->psNext->pszValue)
                dfFontSize = atof(psCssParam->psChild->psNext->pszValue);
              if (dfFontSize <=0.0)
                dfFontSize = 10.0;
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
      if (pszFontFamily) {
        snprintf(szFontName, sizeof(szFontName), "%s", pszFontFamily);
        if (pszFontWeight && strcasecmp(pszFontWeight, "normal") != 0) {
          strlcat(szFontName, "-", sizeof(szFontName));
          strlcat(szFontName, pszFontWeight, sizeof(szFontName));
        }
        if (pszFontStyle && strcasecmp(pszFontStyle, "normal") != 0) {
          strlcat(szFontName, "-", sizeof(szFontName));
          strlcat(szFontName, pszFontStyle, sizeof(szFontName));
        }

        if ((msLookupHashTable(&(psLayer->map->fontset.fonts), szFontName) !=NULL)) {
          bFontSet = 1;
          psLabelObj->font = msStrdup(szFontName);
          psLabelObj->type = MS_TRUETYPE;
          psLabelObj->size = dfFontSize;
        }
      }
      if (!bFontSet) {
        psLabelObj->type = MS_BITMAP;
        psLabelObj->size = MS_MEDIUM;
      }
      /* -------------------------------------------------------------------- */
      /*      parse the label placement.                                      */
      /* -------------------------------------------------------------------- */
      psLabelPlacement = CPLGetXMLNode(psRoot, "LabelPlacement");
      if (psLabelPlacement) {
        psPointPlacement = CPLGetXMLNode(psLabelPlacement,
                                         "PointPlacement");
        psLinePlacement = CPLGetXMLNode(psLabelPlacement,
                                        "LinePlacement");
        if (psPointPlacement)
          ParseTextPointPlacement(psPointPlacement, psClass);
        if (psLinePlacement)
          ParseTextLinePlacement(psLinePlacement, psClass);
      }

      /* -------------------------------------------------------------------- */
      /*      parse the halo parameter.                                       */
      /* -------------------------------------------------------------------- */
      psHalo = CPLGetXMLNode(psRoot, "Halo");
      if (psHalo) {
        psHaloRadius =  CPLGetXMLNode(psHalo, "Radius");
        if (psHaloRadius && psHaloRadius->psChild && psHaloRadius->psChild->pszValue)
          psLabelObj->outlinewidth = atoi(psHaloRadius->psChild->pszValue);

        psHaloFill =  CPLGetXMLNode(psHalo, "Fill");
        if (psHaloFill) {
          psCssParam =  CPLGetXMLNode(psHaloFill, "CssParameter");
          /*sld 1.1 used SvgParameter*/
          if (psCssParam == NULL)
            psCssParam =  CPLGetXMLNode(psHaloFill, "SvgParameter");

          while (psCssParam && psCssParam->pszValue &&
                 (strcasecmp(psCssParam->pszValue, "CssParameter") == 0 ||
                  strcasecmp(psCssParam->pszValue, "SvgParameter") == 0)) {
            pszName = (char*)CPLGetXMLValue(psCssParam, "name", NULL);
            if (pszName) {
              if (strcasecmp(pszName, "fill") == 0) {
                if(psCssParam->psChild && psCssParam->psChild->psNext &&
                    psCssParam->psChild->psNext->pszValue)
                  pszColor = psCssParam->psChild->psNext->pszValue;

                if (pszColor) {
                  nLength = strlen(pszColor);
                  /* expecting hexadecimal ex : #aaaaff */
                  if (nLength == 7 && pszColor[0] == '#') {
                    psLabelObj->outlinecolor.red = msHexToInt(pszColor+1);
                    psLabelObj->outlinecolor.green = msHexToInt(pszColor+3);
                    psLabelObj->outlinecolor.blue = msHexToInt(pszColor+5);
                  }
                }
              }
            }
            psCssParam = psCssParam->psNext;
          }

        }

      }
      /* -------------------------------------------------------------------- */
      /*      Parse the color                                                 */
      /* -------------------------------------------------------------------- */
      psFill = CPLGetXMLNode(psRoot, "Fill");
      if (psFill) {
        psCssParam =  CPLGetXMLNode(psFill, "CssParameter");
        /*sld 1.1 used SvgParameter*/
        if (psCssParam == NULL)
          psCssParam =  CPLGetXMLNode(psFill, "SvgParameter");

        while (psCssParam && psCssParam->pszValue &&
               (strcasecmp(psCssParam->pszValue, "CssParameter") == 0 ||
                strcasecmp(psCssParam->pszValue, "SvgParameter") == 0)) {
          pszName = (char*)CPLGetXMLValue(psCssParam, "name", NULL);
          if (pszName) {
            if (strcasecmp(pszName, "fill") == 0) {
              if(psCssParam->psChild && psCssParam->psChild->psNext &&
                  psCssParam->psChild->psNext->pszValue)
                pszColor = psCssParam->psChild->psNext->pszValue;

              if (pszColor) {
                nLength = strlen(pszColor);
                /* expecting hexadecimal ex : #aaaaff */
                if (nLength == 7 && pszColor[0] == '#') {
                  psLabelObj->color.red = msHexToInt(pszColor+1);
                  psLabelObj->color.green = msHexToInt(pszColor+3);
                  psLabelObj->color.blue = msHexToInt(pszColor+5);
                }
              }
            }
          }
          psCssParam = psCssParam->psNext;
        }
      }

    }/* labelitem */
  }

  return MS_SUCCESS;
}

/************************************************************************/
/*                         ParseTextPointPlacement                      */
/*                                                                      */
/*      point placement node for the text symbolizer.                  */
/************************************************************************/
int ParseTextPointPlacement(CPLXMLNode *psRoot, classObj *psClass)
{
  CPLXMLNode *psAnchor, *psAnchorX, *psAnchorY;
  double dfAnchorX=0, dfAnchorY=0;
  CPLXMLNode *psDisplacement, *psDisplacementX, *psDisplacementY;
  CPLXMLNode *psRotation=NULL, *psPropertyName=NULL;
  char szTmp[100];
  labelObj *psLabelObj = NULL;

  if (!psRoot || !psClass)
    return MS_FAILURE;
  if(psClass->numlabels == 0) {
    if(msGrowClassLabels(psClass) == NULL) return(MS_FAILURE);
    initLabel(psClass->labels[0]);
    psClass->numlabels++;
  }
  psLabelObj = psClass->labels[0];

  /* init the label with the default position */
  psLabelObj->position = MS_CL;

  /* -------------------------------------------------------------------- */
  /*      parse anchor point. see function msSLDParseTextSymbolizer       */
  /*      for documentation.                                              */
  /* -------------------------------------------------------------------- */
  psAnchor = CPLGetXMLNode(psRoot, "AnchorPoint");
  if (psAnchor) {
    psAnchorX = CPLGetXMLNode(psAnchor, "AnchorPointX");
    psAnchorY = CPLGetXMLNode(psAnchor, "AnchorPointY");
    /* psCssParam->psChild->psNext->pszValue) */
    if (psAnchorX &&
        psAnchorX->psChild &&
        psAnchorX->psChild->pszValue &&
        psAnchorY &&
        psAnchorY->psChild &&
        psAnchorY->psChild->pszValue) {
      dfAnchorX = atof(psAnchorX->psChild->pszValue);
      dfAnchorY = atof(psAnchorY->psChild->pszValue);

      if ((dfAnchorX == 0 || dfAnchorX == 0.5 || dfAnchorX == 1) &&
          (dfAnchorY == 0 || dfAnchorY == 0.5 || dfAnchorY == 1)) {
        if (dfAnchorX == 0 && dfAnchorY == 0)
          psLabelObj->position = MS_LL;
        if (dfAnchorX == 0 && dfAnchorY == 0.5)
          psLabelObj->position = MS_CL;
        if (dfAnchorX == 0 && dfAnchorY == 1)
          psLabelObj->position = MS_UL;

        if (dfAnchorX == 0.5 && dfAnchorY == 0)
          psLabelObj->position = MS_LC;
        if (dfAnchorX == 0.5 && dfAnchorY == 0.5)
          psLabelObj->position = MS_CC;
        if (dfAnchorX == 0.5 && dfAnchorY == 1)
          psLabelObj->position = MS_UC;

        if (dfAnchorX == 1 && dfAnchorY == 0)
          psLabelObj->position = MS_LR;
        if (dfAnchorX == 1 && dfAnchorY == 0.5)
          psLabelObj->position = MS_CR;
        if (dfAnchorX == 1 && dfAnchorY == 1)
          psLabelObj->position = MS_UR;
      }
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Parse displacement                                              */
  /* -------------------------------------------------------------------- */
  psDisplacement = CPLGetXMLNode(psRoot, "Displacement");
  if (psDisplacement) {
    psDisplacementX = CPLGetXMLNode(psDisplacement, "DisplacementX");
    psDisplacementY = CPLGetXMLNode(psDisplacement, "DisplacementY");
    /* psCssParam->psChild->psNext->pszValue) */
    if (psDisplacementX &&
        psDisplacementX->psChild &&
        psDisplacementX->psChild->pszValue &&
        psDisplacementY &&
        psDisplacementY->psChild &&
        psDisplacementY->psChild->pszValue) {
      psLabelObj->offsetx = atoi(psDisplacementX->psChild->pszValue);
      psLabelObj->offsety = atoi(psDisplacementY->psChild->pszValue);
    }
  }

  /* -------------------------------------------------------------------- */
  /*      parse rotation.                                                 */
  /* -------------------------------------------------------------------- */
  psRotation = CPLGetXMLNode(psRoot, "Rotation");
  if (psRotation) {
    psPropertyName = CPLGetXMLNode(psRotation, "PropertyName");
    if (psPropertyName) {
      snprintf(szTmp, sizeof(szTmp), "%s", CPLGetXMLValue(psPropertyName, NULL, NULL));
      psLabelObj->bindings[MS_LABEL_BINDING_ANGLE].item = msStrdup(szTmp);
      psLabelObj->numbindings++;
    } else {
      if (psRotation->psChild && psRotation->psChild->pszValue)
        psLabelObj->angle = atof(psRotation->psChild->pszValue);
    }
  }

  return MS_SUCCESS;
}

/************************************************************************/
/*                          ParseTextLinePlacement                      */
/*                                                                      */
/*      Lineplacement node fro the text symbolizer.                     */
/************************************************************************/
int ParseTextLinePlacement(CPLXMLNode *psRoot, classObj *psClass)
{
  CPLXMLNode *psOffset = NULL, *psAligned=NULL;
  labelObj *psLabelObj = NULL;

  if (!psRoot || !psClass)
    return MS_FAILURE;

  if(psClass->numlabels == 0) {
    if(msGrowClassLabels(psClass) == NULL) return(MS_FAILURE);
    initLabel(psClass->labels[0]);
    psClass->numlabels++;
  }
  psLabelObj = psClass->labels[0];

  /*if there is a line placement, we will assume that the
    best setting for mapserver would be for the text to follow
    the line #2806*/
  psLabelObj->anglemode = MS_FOLLOW;

  /*sld 1.1.0 has a parameter IsAligned. default value is true*/
  psAligned = CPLGetXMLNode(psRoot, "IsAligned");
  if (psAligned && psAligned->psChild && psAligned->psChild->pszValue &&
      strcasecmp(psAligned->psChild->pszValue, "false") == 0) {
    psLabelObj->anglemode = MS_NONE;
  }
  psOffset = CPLGetXMLNode(psRoot, "PerpendicularOffset");
  if (psOffset && psOffset->psChild && psOffset->psChild->pszValue) {
    psLabelObj->offsetx = atoi(psOffset->psChild->pszValue);
    psLabelObj->offsety = atoi(psOffset->psChild->pszValue);

    /*if there is a PerpendicularOffset, we will assume that the
      best setting for mapserver would be to use angle=0 and the
      the offset #2806*/
    /* since sld 1.1.0 introduces the IsAligned parameter, only
       set the angles if the parameter is not set*/
    if (!psAligned) {
      psLabelObj->anglemode = MS_NONE;
    }
  }

  return MS_SUCCESS;
}


/************************************************************************/
/*           void msSLDSetColorObject(char *psHexColor, colorObj        */
/*      *psColor)                                                       */
/*                                                                      */
/*      Utility function to exctract rgb values from an hexadecimal     */
/*      color string (format is : #aaff08) and set it in the color      */
/*      object.                                                         */
/************************************************************************/
int msSLDSetColorObject(char *psHexColor, colorObj *psColor)
{
  if (psHexColor && psColor && strlen(psHexColor)== 7 &&
      psHexColor[0] == '#') {

    psColor->red = msHexToInt(psHexColor+1);
    psColor->green = msHexToInt(psHexColor+3);
    psColor->blue= msHexToInt(psHexColor+5);
  }

  return MS_SUCCESS;
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
char *msSLDGenerateSLD(mapObj *map, int iLayer, const char *pszVersion)
{
#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR) || defined(USE_SOS_SVR)

#ifdef USE_OGR


  char szTmp[500];
  int i = 0;
  char *pszTmp = NULL;
  char *pszSLD = NULL;
  char *schemalocation = NULL;
  int sld_version = OWS_VERSION_NOTSET;

  sld_version = msOWSParseVersionString(pszVersion);

  if (sld_version == OWS_VERSION_NOTSET ||
      (sld_version!= OWS_1_0_0 && sld_version!= OWS_1_1_0))
    sld_version = OWS_1_0_0;

  if (map) {
    schemalocation = msEncodeHTMLEntities(msOWSGetSchemasLocation(map));
    if (sld_version ==  OWS_1_0_0)
      snprintf(szTmp, sizeof(szTmp), "<StyledLayerDescriptor version=\"1.0.0\" xmlns=\"http://www.opengis.net/sld\" xmlns:gml=\"http://www.opengis.net/gml\" xmlns:ogc=\"http://www.opengis.net/ogc\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.opengis.net/sld %s/sld/1.0.0/StyledLayerDescriptor.xsd\">\n",schemalocation );
    else
      snprintf(szTmp, sizeof(szTmp), "<StyledLayerDescriptor version=\"1.1.0\" xsi:schemaLocation=\"http://www.opengis.net/sld %s/sld/1.1.0/StyledLayerDescriptor.xsd\" xmlns=\"http://www.opengis.net/sld\" xmlns:ogc=\"http://www.opengis.net/ogc\" xmlns:se=\"http://www.opengis.net/se\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n", schemalocation);

    free(schemalocation);

    pszSLD = msStringConcatenate(pszSLD, szTmp);
    if (iLayer < 0 || iLayer > map->numlayers -1) {
      for (i=0; i<map->numlayers; i++) {
        pszTmp = msSLDGenerateSLDLayer(GET_LAYER(map, i), sld_version);
        if (pszTmp) {
          pszSLD= msStringConcatenate(pszSLD, pszTmp);
          free(pszTmp);
        }
      }
    } else {
      pszTmp = msSLDGenerateSLDLayer(GET_LAYER(map, iLayer), sld_version);
      if (pszTmp) {
        pszSLD = msStringConcatenate(pszSLD, pszTmp);
        free(pszTmp);
      }
    }
    snprintf(szTmp, sizeof(szTmp), "%s", "</StyledLayerDescriptor>\n");
    pszSLD = msStringConcatenate(pszSLD, szTmp);
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
/*      Get an SLD for a style containg a symbol (Mark or external).    */
/************************************************************************/
char *msSLDGetGraphicSLD(styleObj *psStyle, layerObj *psLayer,
                         int bNeedMarkSybol, int nVersion)
{
#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR) || defined(USE_SOS_SVR)

  char *pszSLD = NULL;
  int nSymbol = -1;
  symbolObj *psSymbol = NULL;
  char szTmp[512];
  char *pszURL = NULL;
  char szFormat[4];
  int i = 0, nLength = 0;
  int bFillColor = 0, bStrokeColor = 0, bColorAvailable=0;
  int bGenerateDefaultSymbol = 0;
  char *pszSymbolName= NULL;
  char sNameSpace[10];
  char sCssParam[30];

  sCssParam[0] = '\0';
  if (nVersion > OWS_1_0_0)
    strcpy(sCssParam, "se:SvgParameter");
  else
    strcpy(sCssParam, "CssParameter");

  sNameSpace[0] = '\0';
  if (nVersion > OWS_1_0_0)
    strcpy(sNameSpace, "se:");

  if (psStyle && psLayer && psLayer->map) {
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

    if (nSymbol > 0 && nSymbol < psLayer->map->symbolset.numsymbols) {
      psSymbol =  psLayer->map->symbolset.symbol[nSymbol];
      if (psSymbol->type == MS_SYMBOL_VECTOR ||
          psSymbol->type == MS_SYMBOL_ELLIPSE) {
        /* Mark symbol */
        if (psSymbol->name)

        {
          if (strcasecmp(psSymbol->name, "square") == 0 ||
              strcasecmp(psSymbol->name, "circle") == 0 ||
              strcasecmp(psSymbol->name, "triangle") == 0 ||
              strcasecmp(psSymbol->name, "star") == 0 ||
              strcasecmp(psSymbol->name, "cross") == 0 ||
              strcasecmp(psSymbol->name, "x") == 0)
            pszSymbolName = msStrdup(psSymbol->name);
          else if (strncasecmp(psSymbol->name,
                               "sld_mark_symbol_square", 22) == 0)
            pszSymbolName = msStrdup("square");
          else if (strncasecmp(psSymbol->name,
                               "sld_mark_symbol_triangle", 24) == 0)
            pszSymbolName = msStrdup("triangle");
          else if (strncasecmp(psSymbol->name,
                               "sld_mark_symbol_circle", 22) == 0)
            pszSymbolName = msStrdup("circle");
          else if (strncasecmp(psSymbol->name,
                               "sld_mark_symbol_star", 20) == 0)
            pszSymbolName = msStrdup("star");
          else if (strncasecmp(psSymbol->name,
                               "sld_mark_symbol_cross", 21) == 0)
            pszSymbolName = msStrdup("cross");
          else if (strncasecmp(psSymbol->name,
                               "sld_mark_symbol_x", 17) == 0)
            pszSymbolName = msStrdup("X");



          if (pszSymbolName) {
            colorObj sTmpColor;
            colorObj sTmpStrokeColor;

            snprintf(szTmp, sizeof(szTmp), "<%sGraphic>\n", sNameSpace);
            pszSLD = msStringConcatenate(pszSLD, szTmp);



            snprintf(szTmp, sizeof(szTmp), "<%sMark>\n", sNameSpace);
            pszSLD = msStringConcatenate(pszSLD, szTmp);

            snprintf(szTmp, sizeof(szTmp), "<%sWellKnownName>%s</%sWellKnownName>\n",
                     sNameSpace, pszSymbolName, sNameSpace);
            pszSLD = msStringConcatenate(pszSLD, szTmp);


            if (psStyle->color.red != -1 &&
                psStyle->color.green != -1 &&
                psStyle->color.blue != -1) {
              sTmpColor.red = psStyle->color.red;
              sTmpColor.green = psStyle->color.green;
              sTmpColor.blue = psStyle->color.blue;
              bFillColor =1;
            } else if (psStyle->outlinecolor.red != -1 &&
                       psStyle->outlinecolor.green != -1 &&
                       psStyle->outlinecolor.blue != -1) {
              sTmpStrokeColor.red = psStyle->outlinecolor.red;
              sTmpStrokeColor.green = psStyle->outlinecolor.green;
              sTmpStrokeColor.blue = psStyle->outlinecolor.blue;
              bFillColor++;
            } else {
              sTmpColor.red = 128;
              sTmpColor.green = 128;
              sTmpColor.blue = 128;
              bFillColor =1;
            }
	    if (psStyle->outlinecolor.red != -1 &&
		psStyle->outlinecolor.green != -1 &&
		psStyle->outlinecolor.blue != -1) {
              sTmpStrokeColor.red = psStyle->outlinecolor.red;
              sTmpStrokeColor.green = psStyle->outlinecolor.green;
              sTmpStrokeColor.blue = psStyle->outlinecolor.blue;
              bFillColor++;
            }

            if (psLayer->type == MS_LAYER_POINT) {
              if (psSymbol->filled || bFillColor) {
                snprintf(szTmp, sizeof(szTmp), "<%sFill>\n", sNameSpace);
                pszSLD = msStringConcatenate(pszSLD, szTmp);
                snprintf(szTmp, sizeof(szTmp), "<%s name=\"fill\">#%02x%02x%02x</%s>\n",
                         sCssParam,sTmpColor.red,
                         sTmpColor.green,
                         sTmpColor.blue,
                         sCssParam);
              } else {
                snprintf(szTmp, sizeof(szTmp), "<%sStroke>\n", sNameSpace);
                pszSLD = msStringConcatenate(pszSLD, szTmp);
                snprintf(szTmp, sizeof(szTmp), "<%s name=\"stroke\">#%02x%02x%02x</%s>\n",
                         sCssParam,
                         sTmpColor.red,
                         sTmpColor.green,
                         sTmpColor.blue,
                         sCssParam);
              }
	      if(bFillColor>=2){
		pszSLD = msStringConcatenate(pszSLD, szTmp);
		snprintf(szTmp, sizeof(szTmp), "</%sFill>\n", sNameSpace);
		pszSLD = msStringConcatenate(pszSLD, szTmp);
		snprintf(szTmp, sizeof(szTmp), "<%sStroke>\n", sNameSpace);
		pszSLD = msStringConcatenate(pszSLD, szTmp);
		snprintf(szTmp, sizeof(szTmp), "<%s name=\"stroke\">#%02x%02x%02x</%s>\n",
			 sCssParam,sTmpStrokeColor.red,
			 sTmpStrokeColor.green,
			 sTmpStrokeColor.blue,
			 sCssParam);
		pszSLD = msStringConcatenate(pszSLD, szTmp);
		snprintf(szTmp, sizeof(szTmp), "<%s name=\"stroke-width\">%.2f</%s>\n",
			 sCssParam,psStyle->width,sCssParam);
	      }
            } else {
              if (bFillColor) {
                snprintf(szTmp, sizeof(szTmp), "<%sFill>\n", sNameSpace);
                pszSLD = msStringConcatenate(pszSLD, szTmp);
                snprintf(szTmp, sizeof(szTmp), "<%s name=\"fill\">#%02x%02x%02x</%s>\n",
                         sCssParam,
                         sTmpColor.red,
                         sTmpColor.green,
                         sTmpColor.blue,
                         sCssParam);
              } else {
                snprintf(szTmp, sizeof(szTmp), "<%sStroke>\n", sNameSpace);
                pszSLD = msStringConcatenate(pszSLD, szTmp);
                snprintf(szTmp, sizeof(szTmp), "<%s name=\"stroke\">#%02x%02x%02x</%s>\n",
                         sCssParam,
                         sTmpColor.red,
                         sTmpColor.green,
                         sTmpColor.blue,
                         sCssParam);
              }
            }
            pszSLD = msStringConcatenate(pszSLD, szTmp);

            if ((psLayer->type == MS_LAYER_POINT && psSymbol->filled) ||
                (bFillColor && bFillColor<2))
              snprintf(szTmp, sizeof(szTmp), "</%sFill>\n", sNameSpace);
            else
              snprintf(szTmp, sizeof(szTmp), "</%sStroke>\n", sNameSpace);
            pszSLD = msStringConcatenate(pszSLD, szTmp);

            snprintf(szTmp, sizeof(szTmp), "</%sMark>\n", sNameSpace);
            pszSLD = msStringConcatenate(pszSLD, szTmp);

            if (psStyle->size > 0) {
              snprintf(szTmp, sizeof(szTmp), "<%sSize>%g</%sSize>\n", sNameSpace,
                       psStyle->size, sNameSpace);
              pszSLD = msStringConcatenate(pszSLD, szTmp);
            }

            snprintf(szTmp, sizeof(szTmp), "</%sGraphic>\n", sNameSpace);
            pszSLD = msStringConcatenate(pszSLD, szTmp);

            if (pszSymbolName)
              free(pszSymbolName);
          }
        } else
          bGenerateDefaultSymbol =1;
      } else if (psSymbol->type == MS_SYMBOL_PIXMAP) {
        if (psSymbol->name) {
          pszURL = msLookupHashTable(&(psLayer->metadata), "WMS_SLD_SYMBOL_URL");
          if (!pszURL)
            pszURL = msLookupHashTable(&(psLayer->map->web.metadata), "WMS_SLD_SYMBOL_URL");

          if (pszURL) {
            snprintf(szTmp, sizeof(szTmp), "<%sGraphic>\n", sNameSpace);
            pszSLD = msStringConcatenate(pszSLD, szTmp);



            snprintf(szTmp, sizeof(szTmp), "<%sExternalGraphic>\n", sNameSpace);
            pszSLD = msStringConcatenate(pszSLD, szTmp);

            snprintf(szTmp, sizeof(szTmp), "<%sOnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:type=\"simple\" xlink:href=\"%s%s\"/>\n", sNameSpace,
                     pszURL,psSymbol->imagepath);
            pszSLD = msStringConcatenate(pszSLD, szTmp);
            /* TODO : extract format from symbol */

            szFormat[0] = '\0';
            nLength = strlen(psSymbol->imagepath);
            if (nLength > 3) {
              for (i=0; i<=2; i++)
                szFormat[2-i] = psSymbol->imagepath[nLength-1-i];
              szFormat[3] = '\0';
            }
            if (strlen(szFormat) > 0 &&
                ((strcasecmp (szFormat, "GIF") == 0) ||
                 (strcasecmp (szFormat, "PNG") == 0))) {
              if (strcasecmp (szFormat, "GIF") == 0)
                snprintf(szTmp, sizeof(szTmp), "<%sFormat>image/gif</%sFormat>\n",
                         sNameSpace, sNameSpace);
              else
                snprintf(szTmp, sizeof(szTmp), "<%sFormat>image/png</%sFormat>\n",
                         sNameSpace, sNameSpace);
            } else
              snprintf(szTmp, sizeof(szTmp), "<%sFormat>%s</%sFormat>\n", "image/gif",
                       sNameSpace, sNameSpace);

            pszSLD = msStringConcatenate(pszSLD, szTmp);

            snprintf(szTmp, sizeof(szTmp), "</%sExternalGraphic>\n",  sNameSpace);
            pszSLD = msStringConcatenate(pszSLD, szTmp);

            if (psStyle->size > 0)
              snprintf(szTmp, sizeof(szTmp), "<%sSize>%g</%sSize>\n", sNameSpace, psStyle->size,
                       sNameSpace);
            pszSLD = msStringConcatenate(pszSLD, szTmp);

            snprintf(szTmp, sizeof(szTmp), "</%sGraphic>\n", sNameSpace);
            pszSLD = msStringConcatenate(pszSLD, szTmp);

          }
        }

      }
    }
    if (bGenerateDefaultSymbol) { /* genrate a default square symbol */
      snprintf(szTmp, sizeof(szTmp), "<%sGraphic>\n", sNameSpace);
      pszSLD = msStringConcatenate(pszSLD, szTmp);



      snprintf(szTmp, sizeof(szTmp), "<%sMark>\n", sNameSpace);
      pszSLD = msStringConcatenate(pszSLD, szTmp);

      snprintf(szTmp, sizeof(szTmp), "<%sWellKnownName>%s</%sWellKnownName>\n",
               sNameSpace, "square", sNameSpace);
      pszSLD = msStringConcatenate(pszSLD, szTmp);

      bColorAvailable = 0;
      if (psStyle->color.red != -1 &&
          psStyle->color.green != -1 &&
          psStyle->color.blue != -1) {
        snprintf(szTmp, sizeof(szTmp), "<%sFill>\n", sNameSpace);
        pszSLD = msStringConcatenate(pszSLD, szTmp);
        snprintf(szTmp, sizeof(szTmp), "<%s name=\"fill\">#%02x%02x%02x</%s>\n",
                 sCssParam,
                 psStyle->color.red,
                 psStyle->color.green,
                 psStyle->color.blue,
                 sCssParam);
        pszSLD = msStringConcatenate(pszSLD, szTmp);
        snprintf(szTmp, sizeof(szTmp), "</%sFill>\n", sNameSpace);
        pszSLD = msStringConcatenate(pszSLD, szTmp);
        bColorAvailable = 1;
      }
      if (psStyle->outlinecolor.red != -1 &&
          psStyle->outlinecolor.green != -1 &&
          psStyle->outlinecolor.blue != -1) {
        snprintf(szTmp, sizeof(szTmp), "<%sStroke>\n", sNameSpace);
        pszSLD = msStringConcatenate(pszSLD, szTmp);
        snprintf(szTmp, sizeof(szTmp), "<%s name=\"Stroke\">#%02x%02x%02x</%s>\n",
                 sCssParam,
                 psStyle->outlinecolor.red,
                 psStyle->outlinecolor.green,
                 psStyle->outlinecolor.blue,
                 sCssParam);
        pszSLD = msStringConcatenate(pszSLD, szTmp);
        snprintf(szTmp, sizeof(szTmp), "</%sStroke>\n", sNameSpace);
        pszSLD = msStringConcatenate(pszSLD, szTmp);
        bColorAvailable = 1;
      }
      if (!bColorAvailable) {
        /* default color */
        snprintf(szTmp, sizeof(szTmp), "<%sFill>\n", sNameSpace);
        pszSLD = msStringConcatenate(pszSLD, szTmp);
        snprintf(szTmp, sizeof(szTmp),
                 "<%s name=\"fill\">%s</%s>\n",
                 sCssParam, "#808080", sCssParam);
        pszSLD = msStringConcatenate(pszSLD, szTmp);
        snprintf(szTmp, sizeof(szTmp), "</%sFill>\n", sNameSpace);
        pszSLD = msStringConcatenate(pszSLD, szTmp);
      }

      snprintf(szTmp, sizeof(szTmp), "</%sMark>\n", sNameSpace);
      pszSLD = msStringConcatenate(pszSLD, szTmp);

      if (psStyle->size > 0)
        snprintf(szTmp, sizeof(szTmp), "<%sSize>%g</%sSize>\n", sNameSpace,
                 psStyle->size, sNameSpace);
      else
        snprintf(szTmp,  sizeof(szTmp), "<%sSize>%d</%sSize>\n", sNameSpace,1,sNameSpace);
      pszSLD = msStringConcatenate(pszSLD, szTmp);

      snprintf(szTmp, sizeof(szTmp), "</%sGraphic>\n", sNameSpace);
      pszSLD = msStringConcatenate(pszSLD, szTmp);


    }
  }

  return pszSLD;

#else
  return NULL;

#endif

}




/************************************************************************/
/*                           msSLDGenerateLineSLD                       */
/*                                                                      */
/*      Generate SLD for a Line layer.                                  */
/************************************************************************/
char *msSLDGenerateLineSLD(styleObj *psStyle, layerObj *psLayer, int nVersion)
{
#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR) || defined(USE_SOS_SVR)

  char *pszSLD = NULL;
  char szTmp[100];
  char szHexColor[7];
  int nSymbol = -1;
  int i = 0;
  double dfSize = 1.0;
  char *pszDashArray = NULL;
  char *pszGraphicSLD = NULL;
  char sCssParam[30];
  char sNameSpace[10];

  if ( msCheckParentPointer(psLayer->map,"map")==MS_FAILURE )
    return NULL;

  sCssParam[0] = '\0';
  if (nVersion > OWS_1_0_0)
    strcpy( sCssParam, "se:SvgParameter");
  else
    strcpy( sCssParam, "CssParameter");

  sNameSpace[0] = '\0';
  if (nVersion > OWS_1_0_0)
    strcpy(sNameSpace, "se:");

  snprintf(szTmp, sizeof(szTmp), "<%sLineSymbolizer>\n",  sNameSpace);

  pszSLD = msStringConcatenate(pszSLD, szTmp);

  snprintf(szTmp, sizeof(szTmp), "<%sStroke>\n",  sNameSpace);

  pszSLD = msStringConcatenate(pszSLD, szTmp);

  pszGraphicSLD = msSLDGetGraphicSLD(psStyle, psLayer, 0, nVersion);
  if (pszGraphicSLD) {
    snprintf(szTmp, sizeof(szTmp), "<%sGraphicStroke>\n",  sNameSpace);

    pszSLD = msStringConcatenate(pszSLD, szTmp);

    pszSLD = msStringConcatenate(pszSLD, pszGraphicSLD);

    snprintf(szTmp, sizeof(szTmp), "</%sGraphicStroke>\n",  sNameSpace);

    pszSLD = msStringConcatenate(pszSLD, szTmp);

    free(pszGraphicSLD);
    pszGraphicSLD = NULL;
  }

  if (psStyle->color.red != -1 &&
      psStyle->color.green != -1 &&
      psStyle->color.blue != -1)
    sprintf(szHexColor,"%02x%02x%02x",psStyle->color.red,
            psStyle->color.green,psStyle->color.blue);
  else
    sprintf(szHexColor,"%02x%02x%02x",psStyle->outlinecolor.red,
            psStyle->outlinecolor.green,psStyle->outlinecolor.blue);

  snprintf(szTmp,  sizeof(szTmp),
           "<%s name=\"stroke\">#%s</%s>\n",
           sCssParam, szHexColor, sCssParam);
  pszSLD = msStringConcatenate(pszSLD, szTmp);

  if(psStyle->color.alpha != 255 && psStyle->color.alpha!=-1) {
    snprintf(szTmp, sizeof(szTmp),
             "<%s name=\"stroke-opacity\">%.2f</%s>\n",
             sCssParam, (float)psStyle->color.alpha/255.0, sCssParam);
    pszSLD = msStringConcatenate(pszSLD, szTmp);
  }


  nSymbol = -1;

  if (psStyle->symbol >= 0)
    nSymbol = psStyle->symbol;
  else if (psStyle->symbolname)
    nSymbol = msGetSymbolIndex(&psLayer->map->symbolset,
                               psStyle->symbolname, MS_FALSE);

  if (nSymbol <0)
    dfSize = 1.0;
  else {
    if (psStyle->size > 0)
      dfSize = psStyle->size;
    else if (psStyle->width > 0)
      dfSize = psStyle->width;
    else
      dfSize = 1;
  }

  snprintf(szTmp, sizeof(szTmp),
           "<%s name=\"stroke-width\">%.2f</%s>\n",
           sCssParam, dfSize, sCssParam);
  pszSLD = msStringConcatenate(pszSLD, szTmp);

  /* -------------------------------------------------------------------- */
  /*      dash array                                                      */
  /* -------------------------------------------------------------------- */


  if (psStyle->patternlength > 0) {
    for (i=0; i<psStyle->patternlength; i++) {
      snprintf(szTmp, sizeof(szTmp), "%.2f ", psStyle->pattern[i]);
      pszDashArray = msStringConcatenate(pszDashArray, szTmp);
    }
    snprintf(szTmp, sizeof(szTmp),
             "<%s name=\"stroke-dasharray\">%s</%s>\n",
             sCssParam, pszDashArray, sCssParam);
    pszSLD = msStringConcatenate(pszSLD, szTmp);
  }

  snprintf(szTmp, sizeof(szTmp), "</%sStroke>\n",  sNameSpace);

  pszSLD = msStringConcatenate(pszSLD, szTmp);

  snprintf(szTmp, sizeof(szTmp), "</%sLineSymbolizer>\n",  sNameSpace);

  pszSLD = msStringConcatenate(pszSLD, szTmp);

  return pszSLD;

#else
  return NULL;
#endif
}


/************************************************************************/
/*                         msSLDGeneratePolygonSLD                      */
/*                                                                      */
/*       Generate SLD for a Polygon layer.                              */
/************************************************************************/
char *msSLDGeneratePolygonSLD(styleObj *psStyle, layerObj *psLayer, int nVersion)
{
#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR) || defined(USE_SOS_SVR)

  char szTmp[100];
  char *pszSLD = NULL;
  char szHexColor[7];
  char *pszGraphicSLD = NULL;
  double dfSize;
  char sCssParam[30];
  char sNameSpace[10];

  sCssParam[0] = '\0';
  if (nVersion > OWS_1_0_0)
    strcpy(sCssParam, "se:SvgParameter");
  else
    strcpy(sCssParam, "CssParameter");

  sNameSpace[0] = '\0';
  if (nVersion > OWS_1_0_0)
    strcpy(sNameSpace, "se:");

  snprintf(szTmp, sizeof(szTmp), "<%sPolygonSymbolizer>\n",  sNameSpace);

  pszSLD = msStringConcatenate(pszSLD, szTmp);
  /* fill */
  if (psStyle->color.red != -1 && psStyle->color.green != -1 &&
      psStyle->color.blue != -1) {

    snprintf(szTmp, sizeof(szTmp), "<%sFill>\n",  sNameSpace);

    pszSLD = msStringConcatenate(pszSLD, szTmp);

    pszGraphicSLD = msSLDGetGraphicSLD(psStyle, psLayer, 0, nVersion);
    if (pszGraphicSLD) {
      snprintf(szTmp, sizeof(szTmp), "<%sGraphicFill>\n",  sNameSpace);

      pszSLD = msStringConcatenate(pszSLD, szTmp);

      pszSLD = msStringConcatenate(pszSLD, pszGraphicSLD);

      snprintf(szTmp, sizeof(szTmp), "</%sGraphicFill>\n",  sNameSpace);

      pszSLD = msStringConcatenate(pszSLD, szTmp);

      free(pszGraphicSLD);
      pszGraphicSLD = NULL;
    }

    sprintf(szHexColor,"%02x%02x%02x",psStyle->color.red,
            psStyle->color.green,psStyle->color.blue);

    snprintf(szTmp, sizeof(szTmp),
             "<%s name=\"fill\">#%s</%s>\n",
             sCssParam, szHexColor, sCssParam);
    pszSLD = msStringConcatenate(pszSLD, szTmp);

    if(psStyle->color.alpha != 255 && psStyle->color.alpha!=-1) {
      snprintf(szTmp, sizeof(szTmp),
               "<%s name=\"fill-opacity\">%.2f</%s>\n",
               sCssParam, (float)psStyle->color.alpha/255, sCssParam);
      pszSLD = msStringConcatenate(pszSLD, szTmp);
    }


    snprintf(szTmp, sizeof(szTmp), "</%sFill>\n",  sNameSpace);

    pszSLD = msStringConcatenate(pszSLD, szTmp);
  }
  /* stroke */
  if (psStyle->outlinecolor.red != -1 &&
      psStyle->outlinecolor.green != -1 &&
      psStyle->outlinecolor.blue != -1) {
    snprintf(szTmp, sizeof(szTmp), "<%sStroke>\n",  sNameSpace);
    pszSLD = msStringConcatenate(pszSLD, szTmp);



    /* If there is a symbol to be used for sroke, the color in the */
    /* style sholud be set to -1. Else It won't apply here. */
    if (psStyle->color.red == -1 && psStyle->color.green == -1 &&
        psStyle->color.blue == -1) {
      pszGraphicSLD = msSLDGetGraphicSLD(psStyle, psLayer, 0, nVersion);
      if (pszGraphicSLD) {
        snprintf(szTmp, sizeof(szTmp), "<%sGraphicFill>\n",  sNameSpace);
        pszSLD = msStringConcatenate(pszSLD, szTmp);

        pszSLD = msStringConcatenate(pszSLD, pszGraphicSLD);
        snprintf(szTmp, sizeof(szTmp), "</%sGraphicFill>\n",  sNameSpace);
        pszSLD = msStringConcatenate(pszSLD, szTmp);

        free(pszGraphicSLD);
        pszGraphicSLD = NULL;
      }
    }

    sprintf(szHexColor,"%02x%02x%02x",psStyle->outlinecolor.red,
            psStyle->outlinecolor.green,
            psStyle->outlinecolor.blue);

    snprintf(szTmp, sizeof(szTmp),
             "<%s name=\"stroke\">#%s</%s>\n",
             sCssParam, szHexColor, sCssParam);
    pszSLD = msStringConcatenate(pszSLD, szTmp);

    dfSize = 1.0;
    if (psStyle->size > 0)
      dfSize = psStyle->size;
    else if (psStyle->width > 0)
      dfSize = psStyle->width;

    snprintf(szTmp, sizeof(szTmp),
             "<%s name=\"stroke-width\">%.2f</%s>\n",
             sCssParam,dfSize,sCssParam);
    pszSLD = msStringConcatenate(pszSLD, szTmp);

    snprintf(szTmp, sizeof(szTmp), "</%sStroke>\n",  sNameSpace);
    pszSLD = msStringConcatenate(pszSLD, szTmp);
  }

  snprintf(szTmp, sizeof(szTmp), "</%sPolygonSymbolizer>\n", sNameSpace);
  pszSLD = msStringConcatenate(pszSLD, szTmp);

  return pszSLD;

#else
  return NULL;
#endif
}

/************************************************************************/
/*                          msSLDGeneratePointSLD                       */
/*                                                                      */
/*      Generate SLD for a Point layer.                                 */
/************************************************************************/
char *msSLDGeneratePointSLD(styleObj *psStyle, layerObj *psLayer, int nVersion)
{
#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR) || defined(USE_SOS_SVR)
  char *pszSLD = NULL;
  char *pszGraphicSLD = NULL;
  char szTmp[100];
  char sNameSpace[10];

  sNameSpace[0] = '\0';
  if (nVersion > OWS_1_0_0)
    strcpy(sNameSpace, "se:");

  snprintf(szTmp, sizeof(szTmp), "<%sPointSymbolizer>\n",  sNameSpace);
  pszSLD = msStringConcatenate(pszSLD, szTmp);

  pszGraphicSLD = msSLDGetGraphicSLD(psStyle, psLayer, 1, nVersion);
  if (pszGraphicSLD) {
    pszSLD = msStringConcatenate(pszSLD, pszGraphicSLD);
    free(pszGraphicSLD);
  }

  snprintf(szTmp, sizeof(szTmp), "</%sPointSymbolizer>\n",  sNameSpace);
  pszSLD = msStringConcatenate(pszSLD, szTmp);

  return pszSLD;

#else
  return NULL;

#endif
}



/************************************************************************/
/*                           msSLDGenerateTextSLD                       */
/*                                                                      */
/*      Generate a TextSymboliser SLD xml based on the class's label    */
/*      object.                                                         */
/************************************************************************/
char *msSLDGenerateTextSLD(classObj *psClass, layerObj *psLayer, int nVersion)
{
#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR) || defined(USE_SOS_SVR)
  char *pszSLD = NULL;

  char szTmp[100];
  char **aszFontsParts = NULL;
  int nFontParts = 0;
  char szHexColor[7];
  int nColorRed=-1, nColorGreen=-1, nColorBlue=-1;
  double dfAnchorX = 0.5, dfAnchorY = 0.5;
  int i = 0;
  char sCssParam[30];
  char sNameSpace[10];
  labelObj *psLabelObj = NULL;

  sCssParam[0] = '\0';
  if (nVersion > OWS_1_0_0)
    strcpy(sCssParam, "se:SvgParameter");
  else
    strcpy(sCssParam, "CssParameter");

  sNameSpace[0] = '\0';
  if (nVersion > OWS_1_0_0)
    strcpy(sNameSpace, "se:");


  if (psClass && psLayer && psLayer->labelitem &&
      strlen(psLayer->labelitem) > 0 && psClass->numlabels > 0) {
    psLabelObj = psClass->labels[0];
    snprintf(szTmp, sizeof(szTmp), "<%sTextSymbolizer>\n",  sNameSpace);
    pszSLD = msStringConcatenate(pszSLD, szTmp);

    snprintf(szTmp, sizeof(szTmp), "<%sLabel>%s</%sLabel>\n",  sNameSpace,
             psLayer->labelitem, sNameSpace);
    pszSLD = msStringConcatenate(pszSLD, szTmp);

    /* -------------------------------------------------------------------- */
    /*      only true type fonts are exported. Font name should be          */
    /*      something like arial-bold-italic. There are 3 parts to the      */
    /*      name font-family, font-style (italic, oblique, nomal),          */
    /*      font-weight (bold, normal). These 3 elements are separated      */
    /*      with -.                                                         */
    /* -------------------------------------------------------------------- */
    if (psLabelObj->type == MS_TRUETYPE && psLabelObj->font) {
      aszFontsParts = msStringSplit(psLabelObj->font, '-', &nFontParts);
      if (nFontParts > 0) {
        snprintf(szTmp, sizeof(szTmp), "<%sFont>\n",  sNameSpace);
        pszSLD = msStringConcatenate(pszSLD, szTmp);

        /* assuming first one is font-family */
        snprintf(szTmp, sizeof(szTmp),
                 "<%s name=\"font-family\">%s</%s>\n",
                 sCssParam, aszFontsParts[0], sCssParam);
        pszSLD = msStringConcatenate(pszSLD, szTmp);
        for (i=1; i<nFontParts; i++) {
          if (strcasecmp(aszFontsParts[i], "italic") == 0 ||
              strcasecmp(aszFontsParts[i], "oblique") == 0) {
            snprintf(szTmp, sizeof(szTmp),
                     "<%s name=\"font-style\">%s</%s>\n",
                     sCssParam, aszFontsParts[i], sCssParam);
            pszSLD = msStringConcatenate(pszSLD, szTmp);
          } else if (strcasecmp(aszFontsParts[i], "bold") == 0) {
            snprintf(szTmp, sizeof(szTmp),
                     "<%s name=\"font-weight\">%s</%s>\n",
                     sCssParam,
                     aszFontsParts[i], sCssParam);
            pszSLD = msStringConcatenate(pszSLD, szTmp);
          }
        }
        /* size */
        if (psLabelObj->size > 0) {
          snprintf(szTmp, sizeof(szTmp),
                   "<%s name=\"font-size\">%.2f</%s>\n",
                   sCssParam, psLabelObj->size, sCssParam);
          pszSLD = msStringConcatenate(pszSLD, szTmp);
        }
        snprintf(szTmp, sizeof(szTmp), "</%sFont>\n",  sNameSpace);
        pszSLD = msStringConcatenate(pszSLD, szTmp);

        msFreeCharArray(aszFontsParts, nFontParts);
      }
    }


    /* label placement */
    snprintf(szTmp, sizeof(szTmp), "<%sLabelPlacement>\n<%sPointPlacement>\n",
             sNameSpace, sNameSpace  );
    pszSLD = msStringConcatenate(pszSLD, szTmp);

    snprintf(szTmp, sizeof(szTmp), "<%sAnchorPoint>\n", sNameSpace);
    pszSLD = msStringConcatenate(pszSLD, szTmp);

    if (psLabelObj->position == MS_LL) {
      dfAnchorX =0;
      dfAnchorY = 0;
    } else if (psLabelObj->position == MS_CL) {
      dfAnchorX =0;
      dfAnchorY = 0.5;
    } else if (psLabelObj->position == MS_UL) {
      dfAnchorX =0;
      dfAnchorY = 1;
    }

    else if (psLabelObj->position == MS_LC) {
      dfAnchorX =0.5;
      dfAnchorY = 0;
    } else if (psLabelObj->position == MS_CC) {
      dfAnchorX =0.5;
      dfAnchorY = 0.5;
    } else if (psLabelObj->position == MS_UC) {
      dfAnchorX =0.5;
      dfAnchorY = 1;
    }

    else if (psLabelObj->position == MS_LR) {
      dfAnchorX =1;
      dfAnchorY = 0;
    } else if (psLabelObj->position == MS_CR) {
      dfAnchorX =1;
      dfAnchorY = 0.5;
    } else if (psLabelObj->position == MS_UR) {
      dfAnchorX =1;
      dfAnchorY = 1;
    }
    snprintf(szTmp, sizeof(szTmp), "<%sAnchorPointX>%.1f</%sAnchorPointX>\n",
             sNameSpace, dfAnchorX, sNameSpace);
    pszSLD = msStringConcatenate(pszSLD, szTmp);
    snprintf(szTmp, sizeof(szTmp), "<%sAnchorPointY>%.1f</%sAnchorPointY>\n", sNameSpace,
             dfAnchorY, sNameSpace);
    pszSLD = msStringConcatenate(pszSLD, szTmp);

    snprintf(szTmp, sizeof(szTmp), "</%sAnchorPoint>\n",  sNameSpace);
    pszSLD = msStringConcatenate(pszSLD, szTmp);

    /* displacement */
    if (psLabelObj->offsetx > 0 || psLabelObj->offsety > 0) {
      snprintf(szTmp, sizeof(szTmp), "<%sDisplacement>\n",  sNameSpace);
      pszSLD = msStringConcatenate(pszSLD, szTmp);

      if (psLabelObj->offsetx > 0) {
        snprintf(szTmp, sizeof(szTmp), "<%sDisplacementX>%d</%sDisplacementX>\n",
                 sNameSpace, psLabelObj->offsetx, sNameSpace);
        pszSLD = msStringConcatenate(pszSLD, szTmp);
      }
      if (psLabelObj->offsety > 0) {
        snprintf(szTmp, sizeof(szTmp), "<%sDisplacementY>%d</%sDisplacementY>\n",
                 sNameSpace, psLabelObj->offsety, sNameSpace);
        pszSLD = msStringConcatenate(pszSLD, szTmp);
      }

      snprintf(szTmp,  sizeof(szTmp), "</%sDisplacement>\n",  sNameSpace);
      pszSLD = msStringConcatenate(pszSLD, szTmp);
    }
    /* rotation */
    if (psLabelObj->angle > 0) {
      snprintf(szTmp, sizeof(szTmp), "<%sRotation>%.2f</%sRotation>\n",
               sNameSpace, psLabelObj->angle, sNameSpace);
      pszSLD = msStringConcatenate(pszSLD, szTmp);
    }

    /* TODO : support Halo parameter => shadow */

    snprintf(szTmp, sizeof(szTmp), "</%sPointPlacement>\n</%sLabelPlacement>\n", sNameSpace, sNameSpace);
    pszSLD = msStringConcatenate(pszSLD, szTmp);


    /* color */
    if (psLabelObj->color.red != -1 &&
        psLabelObj->color.green != -1 &&
        psLabelObj->color.blue != -1) {
      nColorRed = psLabelObj->color.red;
      nColorGreen = psLabelObj->color.green;
      nColorBlue = psLabelObj->color.blue;
    } else if (psLabelObj->outlinecolor.red != -1 &&
               psLabelObj->outlinecolor.green != -1 &&
               psLabelObj->outlinecolor.blue != -1) {
      nColorRed = psLabelObj->outlinecolor.red;
      nColorGreen = psLabelObj->outlinecolor.green;
      nColorBlue = psLabelObj->outlinecolor.blue;
    }
    if (nColorRed >= 0 && nColorGreen >= 0  && nColorBlue >=0) {
      snprintf(szTmp, sizeof(szTmp), "<%sFill>\n", sNameSpace );
      pszSLD = msStringConcatenate(pszSLD, szTmp);

      sprintf(szHexColor,"%02x%02x%02x",nColorRed,
              nColorGreen, nColorBlue);

      snprintf(szTmp, sizeof(szTmp),
               "<%s name=\"fill\">#%s</%s>\n",
               sCssParam, szHexColor, sCssParam);
      pszSLD = msStringConcatenate(pszSLD, szTmp);

      snprintf(szTmp, sizeof(szTmp), "</%sFill>\n",  sNameSpace);
      pszSLD = msStringConcatenate(pszSLD, szTmp);
    }

    snprintf(szTmp, sizeof(szTmp), "</%sTextSymbolizer>\n",  sNameSpace);
    pszSLD = msStringConcatenate(pszSLD, szTmp);
  }
  return pszSLD;

#else
  return NULL;
#endif
}


/************************************************************************/
/*                          msSLDGenerateSLDLayer                       */
/*                                                                      */
/*      Genrate an SLD XML string based on the layer's classes.         */
/************************************************************************/
char *msSLDGenerateSLDLayer(layerObj *psLayer, int nVersion)
{
#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR) || defined(USE_SOS_SVR)

#ifdef USE_OGR
  char szTmp[100];
  char *pszTmpName = NULL;
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
       psLayer->type == MS_LAYER_ANNOTATION)) {
    snprintf(szTmp, sizeof(szTmp), "%s\n",  "<NamedLayer>");
    pszFinalSLD = msStringConcatenate(pszFinalSLD, szTmp);

    pszTmp = msOWSLookupMetadata(&(psLayer->metadata), "MO", "name");
    if (pszTmp) {
      pszEncoded = msEncodeHTMLEntities(pszTmp);
      if (nVersion > OWS_1_0_0)
        snprintf(szTmp, sizeof(szTmp), "<se:Name>%s</se:Name>\n", pszEncoded);
      else
        snprintf(szTmp, sizeof(szTmp), "<Name>%s</Name>\n", pszEncoded);
      pszFinalSLD = msStringConcatenate(pszFinalSLD, szTmp);
      msFree(pszEncoded);
    } else if (psLayer->name) {
      pszEncoded = msEncodeHTMLEntities(psLayer->name);
      pszTmpName = (char *)malloc(sizeof(char)*(strlen(pszEncoded)+100));
      if (nVersion > OWS_1_0_0)
        sprintf(pszTmpName, "<se:Name>%s</se:Name>\n", pszEncoded);
      else
        sprintf(pszTmpName, "<Name>%s</Name>\n", pszEncoded);


      msFree(pszEncoded);
      pszFinalSLD = msStringConcatenate(pszFinalSLD, pszTmpName);
      msFree(pszTmpName);
      pszTmpName=NULL;

    } else {
      if (nVersion > OWS_1_0_0)
        snprintf(szTmp, sizeof(szTmp), "<se:Name>%s</se:Name>\n", "NamedLayer");
      else
        snprintf(szTmp, sizeof(szTmp), "<Name>%s</Name>\n", "NamedLayer");
      pszFinalSLD = msStringConcatenate(pszFinalSLD, szTmp);
    }


    snprintf(szTmp,  sizeof(szTmp), "%s\n",  "<UserStyle>");
    pszFinalSLD = msStringConcatenate(pszFinalSLD, szTmp);

    if (nVersion > OWS_1_0_0)
      snprintf(szTmp, sizeof(szTmp), "%s\n",  "<se:FeatureTypeStyle>");
    else
      snprintf(szTmp, sizeof(szTmp), "%s\n",  "<FeatureTypeStyle>");

    pszFinalSLD = msStringConcatenate(pszFinalSLD, szTmp);

    pszWfsFilter = msLookupHashTable(&(psLayer->metadata), "wfs_filter");
    if (pszWfsFilter)
      pszWfsFilterEncoded = msEncodeHTMLEntities(pszWfsFilter);
    if (psLayer->numclasses > 0) {
      for (i=0; i<psLayer->numclasses; i++) {
        if (nVersion > OWS_1_0_0)
          snprintf(szTmp, sizeof(szTmp), "%s\n",  "<se:Rule>");
        else
          snprintf(szTmp, sizeof(szTmp), "%s\n",  "<Rule>");

        pszFinalSLD = msStringConcatenate(pszFinalSLD, szTmp);

        /* if class has a name, use it as the RULE name */
        if (psLayer->class[i]->name) {
          pszEncoded = msEncodeHTMLEntities(psLayer->class[i]->name);
          pszTmpName = (char *)malloc(sizeof(char)*(strlen(pszEncoded)+100));

          if (nVersion > OWS_1_0_0)
            sprintf(pszTmpName, "<se:Name>%s</se:Name>\n",  pszEncoded);
          else
            sprintf(pszTmpName, "<Name>%s</Name>\n",  pszEncoded);

          msFree(pszEncoded);

          pszFinalSLD = msStringConcatenate(pszFinalSLD, pszTmpName);
          msFree(pszTmpName);
          pszTmpName=NULL;
        }
        /* -------------------------------------------------------------------- */
        /*      get the Filter if there is a class expression.                  */
        /* -------------------------------------------------------------------- */
        pszFilter = msSLDGetFilter(psLayer->class[i] ,
                                   pszWfsFilter);/* pszWfsFilterEncoded); */

        if (pszFilter) {
          pszFinalSLD = msStringConcatenate(pszFinalSLD, pszFilter);
          free(pszFilter);
        }
        /* -------------------------------------------------------------------- */
        /*      generate the min/max scale.                                     */
        /* -------------------------------------------------------------------- */
        dfMinScale = -1.0;
        if (psLayer->class[i]->minscaledenom > 0)
          dfMinScale = psLayer->class[i]->minscaledenom;
        else if (psLayer->minscaledenom > 0)
          dfMinScale = psLayer->minscaledenom;
        else if (psLayer->map && psLayer->map->web.minscaledenom > 0)
          dfMinScale = psLayer->map->web.minscaledenom;
        if (dfMinScale > 0) {
          if (nVersion > OWS_1_0_0)
            snprintf(szTmp, sizeof(szTmp), "<se:MinScaleDenominator>%f</se:MinScaleDenominator>\n",
                     dfMinScale);
          else
            snprintf(szTmp, sizeof(szTmp), "<MinScaleDenominator>%f</MinScaleDenominator>\n",
                     dfMinScale);

          pszFinalSLD = msStringConcatenate(pszFinalSLD, szTmp);
        }

        dfMaxScale = -1.0;
        if (psLayer->class[i]->maxscaledenom > 0)
          dfMaxScale = psLayer->class[i]->maxscaledenom;
        else if (psLayer->maxscaledenom > 0)
          dfMaxScale = psLayer->maxscaledenom;
        else if (psLayer->map && psLayer->map->web.maxscaledenom > 0)
          dfMaxScale = psLayer->map->web.maxscaledenom;
        if (dfMaxScale > 0) {
          if (nVersion > OWS_1_0_0)
            snprintf(szTmp, sizeof(szTmp), "<se:MaxScaleDenominator>%f</se:MaxScaleDenominator>\n",
                     dfMaxScale);
          else
            snprintf(szTmp, sizeof(szTmp), "<MaxScaleDenominator>%f</MaxScaleDenominator>\n",
                     dfMaxScale);

          pszFinalSLD = msStringConcatenate(pszFinalSLD, szTmp);
        }



        /* -------------------------------------------------------------------- */
        /*      Line symbolizer.                                                */
        /*                                                                      */
        /*      Right now only generates a stroke element containing css        */
        /*      parameters.                                                     */
        /*      Lines using symbols TODO (specially for dash lines)             */
        /* -------------------------------------------------------------------- */
        if (psLayer->type == MS_LAYER_LINE) {
          for (j=0; j<psLayer->class[i]->numstyles; j++) {
            psStyle = psLayer->class[i]->styles[j];
            pszSLD = msSLDGenerateLineSLD(psStyle, psLayer, nVersion);
            if (pszSLD) {
              pszFinalSLD = msStringConcatenate(pszFinalSLD, pszSLD);
              free(pszSLD);
            }
          }

        } else if (psLayer->type == MS_LAYER_POLYGON) {
          for (j=0; j<psLayer->class[i]->numstyles; j++) {
            psStyle = psLayer->class[i]->styles[j];
            pszSLD = msSLDGeneratePolygonSLD(psStyle, psLayer, nVersion);
            if (pszSLD) {
              pszFinalSLD = msStringConcatenate(pszFinalSLD, pszSLD);
              free(pszSLD);
            }
          }

        } else if (psLayer->type == MS_LAYER_POINT) {
          for (j=0; j<psLayer->class[i]->numstyles; j++) {
            psStyle = psLayer->class[i]->styles[j];
            pszSLD = msSLDGeneratePointSLD(psStyle, psLayer, nVersion);
            if (pszSLD) {
              pszFinalSLD = msStringConcatenate(pszFinalSLD, pszSLD);
              free(pszSLD);
            }
          }

        } else if (psLayer->type == MS_LAYER_ANNOTATION) {
          for (j=0; j<psLayer->class[i]->numstyles; j++) {
            psStyle = psLayer->class[i]->styles[j];
            pszSLD = msSLDGeneratePointSLD(psStyle, psLayer, nVersion);
            if (pszSLD) {
              pszFinalSLD = msStringConcatenate(pszFinalSLD, pszSLD);
              free(pszSLD);
            }
          }

        }
        /* label if it exists */
        pszSLD = msSLDGenerateTextSLD(psLayer->class[i], psLayer, nVersion);
        if (pszSLD) {
          pszFinalSLD = msStringConcatenate(pszFinalSLD, pszSLD);
          free(pszSLD);
        }
        if (nVersion > OWS_1_0_0)
          snprintf(szTmp, sizeof(szTmp), "%s\n",  "</se:Rule>");
        else
          snprintf(szTmp, sizeof(szTmp), "%s\n",  "</Rule>");

        pszFinalSLD = msStringConcatenate(pszFinalSLD, szTmp);


      }
    }
    if (pszWfsFilterEncoded)
      msFree(pszWfsFilterEncoded);
    if (nVersion > OWS_1_0_0)
      snprintf(szTmp, sizeof(szTmp), "%s\n",  "</se:FeatureTypeStyle>");
    else
      snprintf(szTmp, sizeof(szTmp), "%s\n",  "</FeatureTypeStyle>");

    pszFinalSLD = msStringConcatenate(pszFinalSLD, szTmp);

    snprintf(szTmp, sizeof(szTmp), "%s\n",  "</UserStyle>");
    pszFinalSLD = msStringConcatenate(pszFinalSLD, szTmp);

    snprintf(szTmp, sizeof(szTmp), "%s\n",  "</NamedLayer>");
    pszFinalSLD = msStringConcatenate(pszFinalSLD, szTmp);

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
    pszValue = msStrdup("PropertyIsLessThanOrEqualTo");
  else if (strstr(pszExpression, "=~"))
    pszValue = msStrdup("PropertyIsLike");
  else if (strstr(pszExpression, "~*"))
    pszValue = msStrdup("PropertyIsLike");
  else if (strstr(pszExpression, ">=") || strstr(pszExpression, " ge "))
    pszValue = msStrdup("PropertyIsGreaterThanOrEqualTo");
  else if (strstr(pszExpression, "!=") || strstr(pszExpression, " ne "))
    pszValue = msStrdup("PropertyIsNotEqualTo");
  else if (strstr(pszExpression, "=") || strstr(pszExpression, " eq "))
    pszValue = msStrdup("PropertyIsEqualTo");
  else if (strstr(pszExpression, "<") || strstr(pszExpression, " lt "))
    pszValue = msStrdup("PropertyIsLessThan");
  else if (strstr(pszExpression, ">") || strstr(pszExpression, " gt "))
    pszValue = msStrdup("PropertyIsGreaterThan");


  return pszValue;
}

char *msSLDGetLogicalOperator(char *pszExpression)
{

  if (!pszExpression)
    return NULL;

  /* TODO for NOT */

  if(strstr(pszExpression, " AND ") || strstr(pszExpression, "AND("))
    return msStrdup("And");

  if(strstr(pszExpression, " OR ") || strstr(pszExpression, "OR("))
    return msStrdup("Or");

  if(strstr(pszExpression, "NOT ") || strstr(pszExpression, "NOT("))
    return msStrdup("Not");

  return NULL;
}

char *msSLDGetRightExpressionOfOperator(char *pszExpression)
{
  char *pszAnd = NULL, *pszOr = NULL, *pszNot=NULL;

  pszAnd = strstr(pszExpression, " AND ");
  if (!pszAnd)
    pszAnd = strstr(pszExpression, " and ");

  if (pszAnd)
    return msStrdup(pszAnd+4);
  else {
    pszOr = strstr(pszExpression, " OR ");
    if (!pszOr)
      pszOr = strstr(pszExpression, " or ");

    if (pszOr)
      return msStrdup(pszOr+3);
    else {
      pszNot = strstr(pszExpression, "NOT ");
      if (!pszNot)
        pszNot = strstr(pszExpression, "not ");
      if (!pszNot)
        pszNot = strstr(pszExpression, "NOT(");
      if (!pszNot)
        pszNot = strstr(pszExpression, "not(");

      if (pszNot)
        return msStrdup(pszNot+4);
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
  if (strstr(pszExpression, " AND ") || strstr(pszExpression, " and ")) {
    for (i=0; i<nLength-5; i++) {
      if (pszExpression[i] == ' ' &&
          (pszExpression[i+1] == 'A' || pszExpression[i] == 'a') &&
          (pszExpression[i+2] == 'N' || pszExpression[i] == 'n') &&
          (pszExpression[i+3] == 'D' || pszExpression[i] == 'd') &&
          (pszExpression[i+4] == ' '))
        break;
      else {
        pszReturn[iReturn++] = pszExpression[i];
      }
      pszReturn[iReturn] = '\0';
    }
  } else if (strstr(pszExpression, "AND(") || strstr(pszExpression, "and(")) {
    for (i=0; i<nLength-4; i++) {
      if ((pszExpression[i] == 'A' || pszExpression[i] == 'a') &&
          (pszExpression[i+1] == 'N' || pszExpression[i] == 'n') &&
          (pszExpression[i+2] == 'D' || pszExpression[i] == 'd') &&
          (pszExpression[i+3] == '('))
        break;
      else {
        pszReturn[iReturn++] = pszExpression[i];
      }
      pszReturn[iReturn] = '\0';
    }
  } else if (strstr(pszExpression, " OR ") || strstr(pszExpression, " or ")) {
    for (i=0; i<nLength-4; i++) {
      if (pszExpression[i] == ' ' &&
          (pszExpression[i+1] == 'O' || pszExpression[i] == 'o') &&
          (pszExpression[i+2] == 'R' || pszExpression[i] == 'r') &&
          pszExpression[i+3] == ' ')
        break;
      else {
        pszReturn[iReturn++] = pszExpression[i];
      }
      pszReturn[iReturn] = '\0';
    }
  } else if (strstr(pszExpression, "OR(") || strstr(pszExpression, " or(")) {
    for (i=0; i<nLength-3; i++) {
      if ((pszExpression[i] == 'O' || pszExpression[i] == 'o') &&
          (pszExpression[i+1] == 'R' || pszExpression[i] == 'r') &&
          pszExpression[i+2] == '(')
        break;
      else {
        pszReturn[iReturn++] = pszExpression[i];
      }
      pszReturn[iReturn] = '\0';
    }
  } else
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
  pszAnd = strcasestr(pszExpression, " AND ");
  pszOr = strcasestr(pszExpression, " OR ");
  pszNot = strcasestr(pszExpression, "NOT ");

  if (!pszAnd && !pszOr) {
    pszAnd = strcasestr(pszExpression, "AND(");
    pszOr = strcasestr(pszExpression, "OR(");
  }

  if (!pszAnd && !pszOr && !pszNot)
    return 0;

  /* doen not matter how many exactly if there are 2 or more */
  if ((pszAnd && pszOr) || (pszAnd && pszNot) || (pszOr && pszNot))
    return 2;

  if (pszAnd) {
    pszSecondAnd = strcasestr(pszAnd+3, " AND ");
    pszSecondOr = strcasestr(pszAnd+3, " OR ");
  } else if (pszOr) {
    pszSecondAnd = strcasestr(pszOr+2, " AND ");
    pszSecondOr = strcasestr(pszOr+2, " OR ");
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


  if (strcasecmp(pszComparionValue, "PropertyIsEqualTo") == 0) {
    cCompare = '=';
    szCompare[0] = 'e';
    szCompare[1] = 'q';
    szCompare[2] = '\0';

    bOneCharCompare =1;
  }
  if (strcasecmp(pszComparionValue, "PropertyIsNotEqualTo") == 0) {
    szCompare[0] = 'n';
    szCompare[1] = 'e';
    szCompare[2] = '\0';

    szCompare2[0] = '!';
    szCompare2[1] = '=';
    szCompare2[2] = '\0';

    bOneCharCompare =0;
  } else if (strcasecmp(pszComparionValue, "PropertyIsLike") == 0) {
    szCompare[0] = '=';
    szCompare[1] = '~';
    szCompare[2] = '\0';

    szCompare2[0] = '~';
    szCompare2[1] = '*';
    szCompare2[2] = '\0';

    bOneCharCompare =0;
  } else if (strcasecmp(pszComparionValue, "PropertyIsLessThan") == 0) {
    cCompare = '<';
    szCompare[0] = 'l';
    szCompare[1] = 't';
    szCompare[2] = '\0';
    bOneCharCompare =1;
  } else if (strcasecmp(pszComparionValue, "PropertyIsLessThanOrEqualTo") == 0) {
    szCompare[0] = 'l';
    szCompare[1] = 'e';
    szCompare[2] = '\0';

    szCompare2[0] = '<';
    szCompare2[1] = '=';
    szCompare2[2] = '\0';

    bOneCharCompare =0;
  } else if (strcasecmp(pszComparionValue, "PropertyIsGreaterThan") == 0) {
    cCompare = '>';
    szCompare[0] = 'g';
    szCompare[1] = 't';
    szCompare[2] = '\0';
    bOneCharCompare =1;
  } else if (strcasecmp(pszComparionValue, "PropertyIsGreaterThanOrEqualTo") == 0) {
    szCompare[0] = 'g';
    szCompare[1] = 'e';
    szCompare[2] = '\0';

    szCompare2[0] = '>';
    szCompare2[1] = '=';
    szCompare2[2] = '\0';

    bOneCharCompare =0;
  }

  if (bOneCharCompare == 1) {
    aszValues= msStringSplit (pszExpression, cCompare, &nTokens);
    if (nTokens > 1) {
      pszAttributeName = msStrdup(aszValues[0]);
      pszAttributeValue =  msStrdup(aszValues[1]);
      msFreeCharArray(aszValues, nTokens);
    } else {
      nLength = strlen(pszExpression);
      pszAttributeName = (char *)malloc(sizeof(char)*(nLength+1));
      iValue = 0;
      for (i=0; i<nLength-2; i++) {
        if (pszExpression[i] != szCompare[0] &&
            pszExpression[i] != toupper(szCompare[0])) {
          pszAttributeName[iValue++] = pszExpression[i];
        } else {
          if ((pszExpression[i+1] == szCompare[1] ||
               pszExpression[i+1] == toupper(szCompare[1])) &&
              (pszExpression[i+2] == ' ')) {
            iValueIndex = i+3;
            pszAttributeValue = msStrdup(pszExpression+iValueIndex);
            break;
          } else
            pszAttributeName[iValue++] = pszExpression[i];
        }
        pszAttributeName[iValue] = '\0';
      }


    }
  } else if (bOneCharCompare == 0) {
    nLength = strlen(pszExpression);
    pszAttributeName = (char *)malloc(sizeof(char)*(nLength+1));
    iValue = 0;
    for (i=0; i<nLength-2; i++) {
      if ((pszExpression[i] != szCompare[0] ||
           pszExpression[i] != toupper(szCompare[0])) &&
          (pszExpression[i] != szCompare2[0] ||
           pszExpression[i] != toupper(szCompare2[0])))

      {
        pszAttributeName[iValue++] = pszExpression[i];
      } else {
        if (((pszExpression[i+1] == szCompare[1] ||
              pszExpression[i+1] == toupper(szCompare[1])) ||
             (pszExpression[i+1] == szCompare2[1] ||
              pszExpression[i+1] == toupper(szCompare2[1]))) &&
            (pszExpression[i+2] == ' ')) {
          iValueIndex = i+3;
          pszAttributeValue = msStrdup(pszExpression+iValueIndex);
          break;
        } else
          pszAttributeName[iValue++] = pszExpression[i];
      }
      pszAttributeName[iValue] = '\0';
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Return the name of the attribute : It is supposed to be         */
  /*      inside []                                                       */
  /* -------------------------------------------------------------------- */
  if (bReturnName) {
    if (!pszAttributeName)
      return NULL;

    nLength = strlen(pszAttributeName);
    pszFinalAttributeName = (char *)malloc(sizeof(char)*(nLength+1));
    bStartCopy= 0;
    iAtt = 0;
    for (i=0; i<nLength; i++) {
      if (pszAttributeName[i] == ' ' && bStartCopy == 0)
        continue;

      if (pszAttributeName[i] == '[') {
        bStartCopy = 1;
        continue;
      }
      if (pszAttributeName[i] == ']')
        break;
      if (bStartCopy) {
        pszFinalAttributeName[iAtt++] = pszAttributeName[i];
      }
      pszFinalAttributeName[iAtt] = '\0';
    }

    return pszFinalAttributeName;
  } else {

    if (!pszAttributeValue)
      return NULL;
    nLength = strlen(pszAttributeValue);
    pszFinalAttributeValue = (char *)malloc(sizeof(char)*(nLength+1));
    bStartCopy= 0;
    iAtt = 0;
    for (i=0; i<nLength; i++) {
      if (pszAttributeValue[i] == ' ' && bStartCopy == 0)
        continue;

      if (pszAttributeValue[i] == '\'' && bStartCopy == 0) {
        bSingleQuote = 1;
        bStartCopy = 1;
        continue;
      } else if (pszAttributeValue[i] == '"' && bStartCopy == 0) {
        bDoubleQuote = 1;
        bStartCopy = 1;
        continue;
      } else
        bStartCopy =1;

      if (bStartCopy) {
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

    /*trim  for regular expressions*/
    if (pszFinalAttributeValue && strlen(pszFinalAttributeValue) > 2 &&
        strcasecmp(pszComparionValue, "PropertyIsLike") == 0) {
      int len = strlen(pszFinalAttributeValue);
      msStringTrimBlanks(pszFinalAttributeValue);
      if (pszFinalAttributeValue[0] == '/' &&
          (pszFinalAttributeValue[len-1] == '/' ||
           (pszFinalAttributeValue[len-1] == 'i' &&
            pszFinalAttributeValue[len-2] == '/'))) {
        if (pszFinalAttributeValue[len-1] == '/')
          pszFinalAttributeValue[len-1] = '\0';
        else
          pszFinalAttributeValue[len-2] = '\0';

        memmove(pszFinalAttributeValue, pszFinalAttributeValue +
                ((pszFinalAttributeValue[1] == '^') ? 2 : 1), len-1);

        /*replace wild card string .* with * */
        pszFinalAttributeValue = msReplaceSubstring(pszFinalAttributeValue, ".*", "*");
      }
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
  int nLength = 0;
  int nOperators=0;
  char *pszFinalExpression = NULL;
  char *pszComparionValue=NULL, *pszAttibuteName=NULL;
  char *pszAttibuteValue=NULL;
  char *pszLeftExpression=NULL, *pszRightExpression=NULL, *pszOperator=NULL;

  if (!pszExpression || (nLength = strlen(pszExpression)) <=0)
    return NULL;

  pszFinalExpression = (char *)malloc(sizeof(char)*(nLength+1));
  pszFinalExpression[0] = '\0';

  /* -------------------------------------------------------------------- */
  /*      First we check how many logical operators are there :           */
  /*       - if none : It means It is a comparision operator (like =,      */
  /*      >, >= .... We get the comparison value as well as the           */
  /*      attribute and the attribut's value and assign it to the node    */
  /*      passed in argument.                                             */
  /*       - if there is one operator, we assign the operator to the      */
  /*      node and adds the expressions into the left and right nodes.    */
  /* -------------------------------------------------------------------- */
  nOperators = msSLDNumberOfLogicalOperators(pszExpression);
  if (nOperators == 0) {
    if (!psNode)
      psNode = FLTCreateFilterEncodingNode();

    pszComparionValue = msSLDGetComparisonValue(pszExpression);
    pszAttibuteName = msSLDGetAttributeName(pszExpression, pszComparionValue);
    pszAttibuteValue = msSLDGetAttributeValue(pszExpression, pszComparionValue);
    if (pszComparionValue && pszAttibuteName && pszAttibuteValue) {
      psNode->eType = FILTER_NODE_TYPE_COMPARISON;
      psNode->pszValue = msStrdup(pszComparionValue);

      psNode->psLeftNode = FLTCreateFilterEncodingNode();
      psNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
      psNode->psLeftNode->pszValue = msStrdup(pszAttibuteName);

      psNode->psRightNode = FLTCreateFilterEncodingNode();
      psNode->psRightNode->eType = FILTER_NODE_TYPE_LITERAL;
      psNode->psRightNode->pszValue = msStrdup(pszAttibuteValue);

      if (strcasecmp(pszComparionValue, "PropertyIsLike") == 0) {
        psNode->pOther = (FEPropertyIsLike *)malloc(sizeof(FEPropertyIsLike));
        ((FEPropertyIsLike *)psNode->pOther)->bCaseInsensitive = 0;
        ((FEPropertyIsLike *)psNode->pOther)->pszWildCard = msStrdup("*");
        ((FEPropertyIsLike *)psNode->pOther)->pszSingleChar = msStrdup("#");
        ((FEPropertyIsLike *)psNode->pOther)->pszEscapeChar = msStrdup("!");
      }
      free(pszComparionValue);
      free(pszAttibuteName);
      free(pszAttibuteValue);
    }
    return psNode;

  } else if (nOperators == 1) {
    pszOperator = msSLDGetLogicalOperator(pszExpression);
    if (pszOperator) {
      if (!psNode)
        psNode = FLTCreateFilterEncodingNode();

      psNode->eType = FILTER_NODE_TYPE_LOGICAL;
      psNode->pszValue = msStrdup(pszOperator);
      free(pszOperator);

      pszLeftExpression = msSLDGetLeftExpressionOfOperator(pszExpression);
      pszRightExpression = msSLDGetRightExpressionOfOperator(pszExpression);

      if (pszLeftExpression || pszRightExpression) {
        if (pszLeftExpression) {
          pszComparionValue = msSLDGetComparisonValue(pszLeftExpression);
          pszAttibuteName = msSLDGetAttributeName(pszLeftExpression,
                                                  pszComparionValue);
          pszAttibuteValue = msSLDGetAttributeValue(pszLeftExpression,
                             pszComparionValue);

          if (pszComparionValue && pszAttibuteName && pszAttibuteValue) {
            psNode->psLeftNode = FLTCreateFilterEncodingNode();
            psNode->psLeftNode->eType = FILTER_NODE_TYPE_COMPARISON;
            psNode->psLeftNode->pszValue = msStrdup(pszComparionValue);

            psNode->psLeftNode->psLeftNode = FLTCreateFilterEncodingNode();
            psNode->psLeftNode->psLeftNode->eType =
              FILTER_NODE_TYPE_PROPERTYNAME;
            psNode->psLeftNode->psLeftNode->pszValue = msStrdup(pszAttibuteName);

            psNode->psLeftNode->psRightNode = FLTCreateFilterEncodingNode();
            psNode->psLeftNode->psRightNode->eType =
              FILTER_NODE_TYPE_LITERAL;
            psNode->psLeftNode->psRightNode->pszValue =
              msStrdup(pszAttibuteValue);

            free(pszComparionValue);
            free(pszAttibuteName);
            free(pszAttibuteValue);
          }
        }
        if (pszRightExpression) {
          pszComparionValue = msSLDGetComparisonValue(pszRightExpression);
          pszAttibuteName = msSLDGetAttributeName(pszRightExpression,
                                                  pszComparionValue);
          pszAttibuteValue = msSLDGetAttributeValue(pszRightExpression,
                             pszComparionValue);

          if (pszComparionValue && pszAttibuteName && pszAttibuteValue) {
            psNode->psRightNode = FLTCreateFilterEncodingNode();
            psNode->psRightNode->eType = FILTER_NODE_TYPE_COMPARISON;
            psNode->psRightNode->pszValue = msStrdup(pszComparionValue);

            psNode->psRightNode->psLeftNode =
              FLTCreateFilterEncodingNode();
            psNode->psRightNode->psLeftNode->eType =
              FILTER_NODE_TYPE_PROPERTYNAME;
            psNode->psRightNode->psLeftNode->pszValue =
              msStrdup(pszAttibuteName);

            psNode->psRightNode->psRightNode =
              FLTCreateFilterEncodingNode();
            psNode->psRightNode->psRightNode->eType =
              FILTER_NODE_TYPE_LITERAL;
            psNode->psRightNode->psRightNode->pszValue =
              msStrdup(pszAttibuteValue);

            free(pszComparionValue);
            free(pszAttibuteName);
            free(pszAttibuteValue);
          }
        }
      }
    }

    return psNode;
  } else
    return NULL;
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
      psNode->psRightNode && psNode->psRightNode->pszValue) {
    snprintf(szTmp, sizeof(szTmp), "<ogc:%s><ogc:PropertyName>%s</ogc:PropertyName><ogc:Literal>%s</ogc:Literal></ogc:%s>",
             psNode->pszValue, psNode->psLeftNode->pszValue,
             psNode->psRightNode->pszValue, psNode->pszValue);
    pszExpression = msStrdup(szTmp);
  } else if (psNode->eType == FILTER_NODE_TYPE_LOGICAL &&
             psNode->pszValue &&
             ((psNode->psLeftNode && psNode->psLeftNode->pszValue) ||
              (psNode->psRightNode && psNode->psRightNode->pszValue))) {
    snprintf(szTmp, sizeof(szTmp), "<ogc:%s>", psNode->pszValue);
    pszExpression = msStringConcatenate(pszExpression, szTmp);
    if (psNode->psLeftNode) {
      pszTmp = msSLDBuildFilterEncoding(psNode->psLeftNode);
      if (pszTmp) {
        pszExpression = msStringConcatenate(pszExpression, pszTmp);
        free(pszTmp);
      }
    }
    if (psNode->psRightNode) {
      pszTmp = msSLDBuildFilterEncoding(psNode->psRightNode);
      if (pszTmp) {
        pszExpression = msStringConcatenate(pszExpression, pszTmp);
        free(pszTmp);
      }
    }
    snprintf(szTmp,  sizeof(szTmp), "</ogc:%s>", psNode->pszValue);
    pszExpression = msStringConcatenate(pszExpression, szTmp);
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

  if (psNode) {
    pszFLTExpression = msSLDBuildFilterEncoding(psNode);
    if (pszFLTExpression) {
      pszTmp = msStringConcatenate(pszTmp, "<ogc:Filter>");
      if (pszWfsFilter) {
        pszTmp = msStringConcatenate(pszTmp, "<ogc:And>");
        pszTmp = msStringConcatenate(pszTmp, (char *)pszWfsFilter);
      }
      pszTmp = msStringConcatenate(pszTmp, pszFLTExpression);

      if (pszWfsFilter)
        pszTmp = msStringConcatenate(pszTmp, "</ogc:And>");

      pszTmp = msStringConcatenate(pszTmp, "</ogc:Filter>\n");

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

  aszElements = msStringSplit(pszExpression, ' ', &nElements);

  szFinalAtt[0] = '\0';
  szFinalValue[0] = '\0';
  for (i=0; i<nElements; i++) {
    if (strcasecmp(aszElements[i], "=") == 0 ||
        strcasecmp(aszElements[i], "eq") == 0) {
      if (i > 0 && i < nElements-1) {
        snprintf(szAttribute, sizeof(szAttribute), "%s", aszElements[i-1]);
        snprintf(szValue, sizeof(szValue), "%s", aszElements[i+1]);

        /* parse attribute */
        nLength = strlen(szAttribute);
        if (nLength > 0) {
          iAtt = 0;
          for (i=0; i<nLength; i++) {
            if (szAttribute[i] == '[') {
              bStartCopy = 1;
              continue;
            }
            if (szAttribute[i] == ']')
              break;
            if (bStartCopy) {
              szFinalAtt[iAtt] = szAttribute[i];
              iAtt++;
            }
            szFinalAtt[iAtt] = '\0';
          }
        }

        /* parse value */
        nLength = strlen(szValue);
        if (nLength > 0) {
          if (szValue[0] == '\'')
            bSinglequote = 1;
          else if (szValue[0] == '\"')
            bDoublequote = 1;
          else
            snprintf(szFinalValue, sizeof(szFinalValue), "%s", szValue);

          iVal = 0;
          if (bSinglequote || bDoublequote) {
            for (i=1; i<nLength-1; i++)
              szFinalValue[iVal++] = szValue[i];

            szFinalValue[iVal] = '\0';
          }
        }
      }
      if (strlen(szFinalAtt) > 0 && strlen(szFinalValue) >0) {
        snprintf(szBuffer, sizeof(szBuffer), "<ogc:Filter><ogc:PropertyIsEqualTo><ogc:PropertyName>%s</ogc:PropertyName><ogc:Literal>%s</ogc:Literal></ogc:PropertyIsEqualTo></ogc:Filter>",
                 szFinalAtt, szFinalValue);
        pszFilter = msStrdup(szBuffer);
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

  while (i < nLength) {
    if (pszRegex[i] != '.') {
      szBuffer[iBuffer++] = pszRegex[i];
      i++;
    } else {
      if (i<nLength-1 && pszRegex[i+1] == '*') {
        szBuffer[iBuffer++] = '*';
        i = i+2;
      } else {
        szBuffer[iBuffer++] =  pszRegex[i];
        i++;
      }
    }
  }
  szBuffer[iBuffer] = '\0';

  return msStrdup(szBuffer);
}



/************************************************************************/
/*                              msSLDGetFilter                          */
/*                                                                      */
/*      Get the corresponding ogc Filter based on the class             */
/*      expression. TODO : move function to mapogcfilter.c when         */
/*      finished.                                                       */
/************************************************************************/
char *msSLDGetFilter(classObj *psClass, const char *pszWfsFilter)
{
  char *pszFilter = NULL;
  char szBuffer[500];
  char *pszOgcFilter = NULL;

  if (psClass && psClass->expression.string) {
    /* string expression */
    if (psClass->expression.type == MS_STRING) {
      if (psClass->layer && psClass->layer->classitem) {
        if (pszWfsFilter)
          snprintf(szBuffer, sizeof(szBuffer), "<ogc:Filter><ogc:And>%s<ogc:PropertyIsEqualTo><ogc:PropertyName>%s</ogc:PropertyName><ogc:Literal>%s</ogc:Literal></ogc:PropertyIsEqualTo></ogc:And></ogc:Filter>\n",
                   pszWfsFilter, psClass->layer->classitem, psClass->expression.string);
        else
          snprintf(szBuffer, sizeof(szBuffer), "<ogc:Filter><ogc:PropertyIsEqualTo><ogc:PropertyName>%s</ogc:PropertyName><ogc:Literal>%s</ogc:Literal></ogc:PropertyIsEqualTo></ogc:Filter>\n",
                   psClass->layer->classitem, psClass->expression.string);
        pszFilter = msStrdup(szBuffer);
      }
    } else if (psClass->expression.type == MS_EXPRESSION) {
      pszFilter = msSLDParseLogicalExpression(psClass->expression.string,
                                              pszWfsFilter);
    } else if (psClass->expression.type == MS_REGEX) {
      if (psClass->layer && psClass->layer->classitem && psClass->expression.string) {
        pszOgcFilter = msSLDConvertRegexExpToOgcIsLike(psClass->expression.string);

        if (pszWfsFilter)
          snprintf(szBuffer, sizeof(szBuffer), "<ogc:Filter><ogc:And>%s<ogc:PropertyIsLike wildCard=\"*\" singleChar=\".\" escape=\"\\\"><ogc:PropertyName>%s</ogc:PropertyName><ogc:Literal>%s</ogc:Literal></ogc:PropertyIsLike></ogc:And></ogc:Filter>\n",
                   pszWfsFilter, psClass->layer->classitem, pszOgcFilter);
        else
          snprintf(szBuffer, sizeof(szBuffer), "<ogc:Filter><ogc:PropertyIsLike wildCard=\"*\" singleChar=\".\" escape=\"\\\"><ogc:PropertyName>%s</ogc:PropertyName><ogc:Literal>%s</ogc:Literal></ogc:PropertyIsLike></ogc:Filter>\n",
                   psClass->layer->classitem, pszOgcFilter);

        free(pszOgcFilter);

        pszFilter = msStrdup(szBuffer);
      }
    }
  } else if (pszWfsFilter) {
    snprintf(szBuffer, sizeof(szBuffer), "<ogc:Filter>%s</ogc:Filter>\n", pszWfsFilter);
    pszFilter = msStrdup(szBuffer);
  }
  return pszFilter;
}

#endif

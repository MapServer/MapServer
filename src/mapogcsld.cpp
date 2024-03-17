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
#include "mapows.h"
#include "mapcopy.h"
#include "cpl_string.h"

extern "C" {
extern int yyparse(parseObj *);
}

static inline void IGUR_sizet(size_t ignored) {
  (void)ignored;
} /* Ignore GCC Unused Result */

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
int msSLDApplySLDURL(mapObj *map, const char *szURL, int iLayer,
                     const char *pszStyleLayerName, char **ppszLayerNames) {
  /* needed for libcurl function msHTTPGetFile in maphttp.c */
#if defined(USE_CURL)

  char *pszSLDTmpFile = NULL;
  int status = 0;
  char *pszSLDbuf = NULL;
  FILE *fp = NULL;
  int nStatus = MS_FAILURE;

  if (map && szURL) {
    map->sldurl = (char *)szURL;
    pszSLDTmpFile = msTmpFile(map, map->mappath, NULL, "sld.xml");
    if (pszSLDTmpFile == NULL) {
      pszSLDTmpFile = msTmpFile(map, NULL, NULL, "sld.xml");
    }
    if (pszSLDTmpFile == NULL) {
      msSetError(
          MS_WMSERR,
          "Could not determine temporary file. Please make sure that the "
          "temporary path is set. The temporary path can be defined for "
          "example by setting TEMPPATH in the map file. Please check the "
          "MapServer documentation on temporary path settings.",
          "msSLDApplySLDURL()");
    } else {
      int nMaxRemoteSLDBytes;
      const char *pszMaxRemoteSLDBytes = msOWSLookupMetadata(
          &(map->web.metadata), "MO", "remote_sld_max_bytes");
      if (!pszMaxRemoteSLDBytes) {
        nMaxRemoteSLDBytes = 1024 * 1024; /* 1 megaByte */
      } else {
        nMaxRemoteSLDBytes = atoi(pszMaxRemoteSLDBytes);
      }
      if (msHTTPGetFile(szURL, pszSLDTmpFile, &status, -1, 0, 0,
                        nMaxRemoteSLDBytes) == MS_SUCCESS) {
        if ((fp = fopen(pszSLDTmpFile, "rb")) != NULL) {
          int nBufsize = 0;
          fseek(fp, 0, SEEK_END);
          nBufsize = ftell(fp);
          if (nBufsize > 0) {
            rewind(fp);
            pszSLDbuf = (char *)malloc((nBufsize + 1) * sizeof(char));
            if (pszSLDbuf == NULL) {
              msSetError(MS_MEMERR, "Failed to open SLD file.",
                         "msSLDApplySLDURL()");
            } else {
              IGUR_sizet(fread(pszSLDbuf, 1, nBufsize, fp));
              pszSLDbuf[nBufsize] = '\0';
            }
          } else {
            msSetError(MS_WMSERR, "Could not open SLD %s as it appears empty",
                       "msSLDApplySLDURL", szURL);
          }
          fclose(fp);
          unlink(pszSLDTmpFile);
        }
      } else {
        unlink(pszSLDTmpFile);
        msSetError(
            MS_WMSERR,
            "Could not open SLD %s and save it in a temporary file. Please "
            "make sure that the sld url is valid and that the temporary path "
            "is set. The temporary path can be defined for example by setting "
            "TEMPPATH in the map file. Please check the MapServer "
            "documentation on temporary path settings.",
            "msSLDApplySLDURL", szURL);
      }
      msFree(pszSLDTmpFile);
      if (pszSLDbuf)
        nStatus = msSLDApplySLD(map, pszSLDbuf, iLayer, pszStyleLayerName,
                                ppszLayerNames);
    }
    map->sldurl = NULL;
  }

  msFree(pszSLDbuf);

  return nStatus;

#else
  msSetError(MS_MISCERR, "WMS/WFS client support is not enabled .",
             "msSLDApplySLDURL()");
  return (MS_FAILURE);
#endif
}

/************************************************************************/
/*                             msSLDApplyFromFile                       */
/*                                                                      */
/*      Apply SLD from a file on disk to the layerObj                   */
/************************************************************************/
int msSLDApplyFromFile(mapObj *map, layerObj *layer, const char *filename) {

  FILE *fp = NULL;

  int nStatus = MS_FAILURE;

  /* open SLD file */
  char szPath[MS_MAXPATHLEN];
  char *pszSLDbuf = NULL;
  const char *realpath = msBuildPath(szPath, map->mappath, filename);

  if ((fp = fopen(realpath, "rb")) != NULL) {
    int nBufsize = 0;
    fseek(fp, 0, SEEK_END);
    nBufsize = ftell(fp);
    if (nBufsize > 0) {
      rewind(fp);
      pszSLDbuf = (char *)malloc((nBufsize + 1) * sizeof(char));
      if (pszSLDbuf == NULL) {
        msSetError(MS_MEMERR, "Failed to read SLD file.",
                   "msSLDApplyFromFile()");
      } else {
        IGUR_sizet(fread(pszSLDbuf, 1, nBufsize, fp));
        pszSLDbuf[nBufsize] = '\0';
      }
    } else {
      msSetError(MS_IOERR, "Could not open SLD %s as it appears empty",
                 "msSLDApplyFromFile", realpath);
    }
    fclose(fp);
  }
  if (pszSLDbuf) {
    // if not set, then use the first NamedStyle in the SLD file
    // even if the names don't match
    const char *pszMetadataName = "SLD_USE_FIRST_NAMEDLAYER";
    const char *pszValue =
        msLookupHashTable(&(layer->metadata), pszMetadataName);
    if (pszValue == NULL) {
      msInsertHashTable(&(layer->metadata), pszMetadataName, "true");
    }
    nStatus = msSLDApplySLD(map, pszSLDbuf, layer->index, NULL, NULL);
  } else {
    msSetError(MS_IOERR, "Invalid SLD filename: \"%s\".",
               "msSLDApplyFromFile()", realpath);
  }

  msFree(pszSLDbuf);
  return nStatus;
}

#if defined(USE_WMS_SVR) || defined(USE_WFS_SVR) || defined(USE_WCS_SVR) ||    \
    defined(USE_SOS_SVR)
/* -------------------------------------------------------------------- */
/*      If the same layer is given more that once, we need to           */
/*      duplicate it.                                                   */
/* -------------------------------------------------------------------- */
static void msSLDApplySLD_DuplicateLayers(mapObj *map, int nSLDLayers,
                                          layerObj *pasSLDLayers) {
  int m;
  for (m = 0; m < nSLDLayers; m++) {
    int l;
    int nIndex = msGetLayerIndex(map, pasSLDLayers[m].name);
    if (pasSLDLayers[m].name == NULL)
      continue;
    if (nIndex < 0)
      continue;
    for (l = m + 1; l < nSLDLayers; l++) {
      if (pasSLDLayers[l].name == NULL)
        continue;
      if (strcasecmp(pasSLDLayers[m].name, pasSLDLayers[l].name) == 0) {
        layerObj *psTmpLayer = (layerObj *)malloc(sizeof(layerObj));
        char tmpId[128];

        initLayer(psTmpLayer, map);
        msCopyLayer(psTmpLayer, GET_LAYER(map, nIndex));
        /* open the source layer */
        if (!psTmpLayer->vtable) {
          if (msInitializeVirtualTable(psTmpLayer) != MS_SUCCESS) {
            MS_REFCNT_DECR(psTmpLayer);
            continue;
          }
        }

        /*make the name unique*/
        snprintf(tmpId, sizeof(tmpId), "%lx_%x_%d", (long)time(NULL),
                 (int)getpid(), map->numlayers);
        if (psTmpLayer->name)
          msFree(psTmpLayer->name);
        psTmpLayer->name = msStrdup(tmpId);
        msFree(pasSLDLayers[l].name);
        pasSLDLayers[l].name = msStrdup(tmpId);
        msInsertLayer(map, psTmpLayer, -1);
        MS_REFCNT_DECR(psTmpLayer);
      }
    }
  }
}
#endif

static int msApplySldLayerToMapLayer(layerObj *sldLayer, layerObj *lp) {

  if (sldLayer->numclasses > 0) {
    int iClass = 0;
    bool bSLDHasNamedClass = false;

    lp->type = sldLayer->type;
    lp->rendermode = MS_ALL_MATCHING_CLASSES;

    for (int k = 0; k < sldLayer->numclasses; k++) {
      if (sldLayer->_class[k]->group) {
        bSLDHasNamedClass = true;
        break;
      }
    }

    for (int k = 0; k < lp->numclasses; k++) {
      if (lp->_class[k] != NULL) {
        lp->_class[k]->layer = NULL;
        if (freeClass(lp->_class[k]) == MS_SUCCESS) {
          msFree(lp->_class[k]);
          lp->_class[k] = NULL;
        }
      }
    }
    lp->numclasses = 0;

    if (bSLDHasNamedClass && sldLayer->classgroup) {
      /* Set the class group to the class that has UserStyle.IsDefaultf
       */
      msFree(lp->classgroup);
      lp->classgroup = msStrdup(sldLayer->classgroup);
    } else {
      /*unset the classgroup on the layer if it was set. This allows the
      layer to render with all the classes defined in the SLD*/
      msFree(lp->classgroup);
      lp->classgroup = NULL;
    }

    for (int k = 0; k < sldLayer->numclasses; k++) {
      if (msGrowLayerClasses(lp) == NULL)
        return MS_FAILURE;

      initClass(lp->_class[iClass]);
      msCopyClass(lp->_class[iClass], sldLayer->_class[k], NULL);
      lp->_class[iClass]->layer = lp;
      lp->numclasses++;

      /*aliases may have been used as part of the sld text symbolizer
        for label element. Try to process it if that is the case #3114*/

      const int layerWasOpened = msLayerIsOpen(lp);

      if (layerWasOpened || (msLayerOpen(lp) == MS_SUCCESS &&
                             msLayerGetItems(lp) == MS_SUCCESS)) {

        if (lp->_class[iClass]->text.string) {
          for (int z = 0; z < lp->numitems; z++) {
            if (!lp->items[z] || strlen(lp->items[z]) == 0)
              continue;

            char *pszTmp1 = msStrdup(lp->_class[iClass]->text.string);
            const char *pszFullName = msOWSLookupMetadata(
                &(lp->metadata), "G",
                std::string(lp->items[z]).append("_alias").c_str());

            if (pszFullName != NULL && (strstr(pszTmp1, pszFullName) != NULL)) {
              pszTmp1 = msReplaceSubstring(pszTmp1, pszFullName, lp->items[z]);
              std::string osTmp("(");
              osTmp.append(pszTmp1).append(")");
              msLoadExpressionString(&(lp->_class[iClass]->text),
                                     osTmp.c_str());
            }
            msFree(pszTmp1);
          }
        }
        if (!layerWasOpened) { // don't close the layer if already
                               // open
          msLayerClose(lp);
        }
      }
      iClass++;
    }
  } else {
    /*this is probably an SLD that uses Named styles*/
    if (sldLayer->classgroup) {
      int k;
      for (k = 0; k < lp->numclasses; k++) {
        if (lp->_class[k]->group &&
            strcasecmp(lp->_class[k]->group, sldLayer->classgroup) == 0)
          break;
      }
      if (k < lp->numclasses) {
        msFree(lp->classgroup);
        lp->classgroup = msStrdup(sldLayer->classgroup);
      } else {
        /* TODO  we throw an exception ?*/
      }
    }
  }
  if (sldLayer->labelitem) {
    if (lp->labelitem)
      free(lp->labelitem);

    lp->labelitem = msStrdup(sldLayer->labelitem);
  }

  if (sldLayer->classitem) {
    if (lp->classitem)
      free(lp->classitem);

    lp->classitem = msStrdup(sldLayer->classitem);
  }

  /* opacity for sld raster */
  if (lp->type == MS_LAYER_RASTER && sldLayer->compositer &&
      sldLayer->compositer->opacity != 100)
    msSetLayerOpacity(lp, sldLayer->compositer->opacity);

  /* mark as auto-generate SLD */
  if (lp->connectiontype == MS_WMS)
    msInsertHashTable(&(lp->metadata), "wms_sld_body", "auto");

  /* The SLD might have a FeatureTypeConstraint */
  if (sldLayer->filter.type == MS_EXPRESSION) {
    if (lp->filter.string && lp->filter.type == MS_EXPRESSION) {
      char *pszBuffer = msStringConcatenate(NULL, "((");
      pszBuffer = msStringConcatenate(pszBuffer, lp->filter.string);
      pszBuffer = msStringConcatenate(pszBuffer, ") AND (");
      pszBuffer = msStringConcatenate(pszBuffer, sldLayer->filter.string);
      pszBuffer = msStringConcatenate(pszBuffer, "))");
      msFreeExpression(&lp->filter);
      msInitExpression(&lp->filter);
      lp->filter.string = pszBuffer;
      lp->filter.type = MS_EXPRESSION;
    } else {
      msFreeExpression(&lp->filter);
      msInitExpression(&lp->filter);
      lp->filter.string = msStrdup(sldLayer->filter.string);
      lp->filter.type = MS_EXPRESSION;
    }
  }

  /*in some cases it would make sense to concatenate all the class
    expressions and use it to set the filter on the layer. This
    could increase performance. Will do it for db types layers
    #2840*/
  if (lp->filter.string == NULL ||
      (lp->filter.string && lp->filter.type == MS_STRING && !lp->filteritem)) {
    if (lp->connectiontype == MS_POSTGIS ||
        lp->connectiontype == MS_ORACLESPATIAL ||
        lp->connectiontype == MS_PLUGIN) {
      if (lp->numclasses > 0) {
        /* check first that all classes have an expression type.
          That is the only way we can concatenate them and set the
          filter expression */
        int k;
        for (k = 0; k < lp->numclasses; k++) {
          if (lp->_class[k]->expression.type != MS_EXPRESSION)
            break;
        }
        if (k == lp->numclasses) {
          char szTmp[512];
          char *pszBuffer = NULL;
          for (k = 0; k < lp->numclasses; k++) {
            if (pszBuffer == NULL)
              snprintf(szTmp, sizeof(szTmp), "%s",
                       "(("); /* we a building a string expression,
                                 explicitly set type below */
            else
              snprintf(szTmp, sizeof(szTmp), "%s", " OR ");

            pszBuffer = msStringConcatenate(pszBuffer, szTmp);
            pszBuffer = msStringConcatenate(pszBuffer,
                                            lp->_class[k]->expression.string);
          }

          snprintf(szTmp, sizeof(szTmp), "%s", "))");
          pszBuffer = msStringConcatenate(pszBuffer, szTmp);

          msFreeExpression(&lp->filter);
          msInitExpression(&lp->filter);
          lp->filter.string = msStrdup(pszBuffer);
          lp->filter.type = MS_EXPRESSION;

          msFree(pszBuffer);
        }
      }
    }
  }

  return MS_SUCCESS;
}

/************************************************************************/
/*                              msSLDApplySLD                           */
/*                                                                      */
/*      Parses the SLD into array of layers. Go through the map and     */
/*      compare the SLD layers and the map layers using the name. If    */
/*      they have the same name, copy the classes associated with       */
/*      the SLD layers onto the map layers.                             */
/************************************************************************/
int msSLDApplySLD(mapObj *map, const char *psSLDXML, int iLayer,
                  const char *pszStyleLayerName, char **ppszLayerNames) {
#if defined(USE_WMS_SVR) || defined(USE_WFS_SVR) || defined(USE_WCS_SVR) ||    \
    defined(USE_SOS_SVR)
  int nSLDLayers = 0;
  layerObj *pasSLDLayers = NULL;
  int nStatus = MS_SUCCESS;
  /*const char *pszSLDNotSupported = NULL;*/

  pasSLDLayers = msSLDParseSLD(map, psSLDXML, &nSLDLayers);
  if (pasSLDLayers == NULL) {
    errorObj *psError = msGetErrorObj();
    if (psError && psError->code != MS_NOERR)
      return MS_FAILURE;
  }

  if (pasSLDLayers && nSLDLayers > 0) {
    int i;

    msSLDApplySLD_DuplicateLayers(map, nSLDLayers, pasSLDLayers);

    for (i = 0; i < map->numlayers; i++) {
      layerObj *lp = NULL;
      const char *pszWMSLayerName = NULL;
      int j;
      int bUseSpecificLayer = 0;

      if (iLayer >= 0 && iLayer < map->numlayers) {
        i = iLayer;
        bUseSpecificLayer = 1;
      }

      lp = GET_LAYER(map, i);

      /* compare layer name to wms_name as well */
      pszWMSLayerName = msOWSLookupMetadata(&(lp->metadata), "MO", "name");

      bool bSldApplied = false;

      for (j = 0; j < nSLDLayers; j++) {
        layerObj *sldLayer = &pasSLDLayers[j];

        /* --------------------------------------------------------------------
         */
        /*      copy :  - class */
        /*              - layer's labelitem */
        /* --------------------------------------------------------------------
         */
        if ((sldLayer->name && pszStyleLayerName == NULL &&
             ((strcasecmp(lp->name, sldLayer->name) == 0 ||
               (pszWMSLayerName &&
                strcasecmp(pszWMSLayerName, sldLayer->name) == 0)) ||
              (lp->group && strcasecmp(lp->group, sldLayer->name) == 0))) ||
            (bUseSpecificLayer && pszStyleLayerName && sldLayer->name &&
             strcasecmp(sldLayer->name, pszStyleLayerName) == 0)) {
#ifdef notdef
          /*this is a test code if we decide to flag some layers as not
           * supporting SLD*/
          pszSLDNotSupported =
              msOWSLookupMetadata(&(lp->metadata), "M", "SLD_NOT_SUPPORTED");
          if (pszSLDNotSupported) {
            msSetError(MS_WMSERR, "Layer %s does not support SLD",
                       "msSLDApplySLD", sldLayer->name);
            nsStatus = MS_FAILURE;
            goto sld_cleanup;
          }
#endif
          if (msApplySldLayerToMapLayer(sldLayer, lp) == MS_FAILURE) {
            nStatus = MS_FAILURE;
            goto sld_cleanup;
          };
          bSldApplied = true;
          break;
        }
      }
      if (bUseSpecificLayer) {
        if (!bSldApplied) {
          // there was no name match between the map layer and the SLD named
          // layer - check if we should apply the first SLD layer anyway
          const char *pszSLDUseFirstNamedLayer =
              msLookupHashTable(&(lp->metadata), "SLD_USE_FIRST_NAMEDLAYER");
          if (pszSLDUseFirstNamedLayer) {
            if (strcasecmp(pszSLDUseFirstNamedLayer, "true") == 0) {
              layerObj *firstSldLayer = &pasSLDLayers[0];
              if (msApplySldLayerToMapLayer(firstSldLayer, lp) == MS_FAILURE) {
                nStatus = MS_FAILURE;
                goto sld_cleanup;
              };
            }
          }
        }
        break;
      }
    }

    /* -------------------------------------------------------------------- */
    /*      if needed return a comma separated list of the layers found     */
    /*      in the sld.                                                     */
    /* -------------------------------------------------------------------- */
    if (ppszLayerNames) {
      char *pszTmp = NULL;
      for (i = 0; i < nSLDLayers; i++) {
        if (pasSLDLayers[i].name) {
          if (pszTmp != NULL)
            pszTmp = msStringConcatenate(pszTmp, ",");
          pszTmp = msStringConcatenate(pszTmp, pasSLDLayers[i].name);
        }
      }
      *ppszLayerNames = pszTmp;
    }
  }

  nStatus = MS_SUCCESS;

sld_cleanup:

  if (pasSLDLayers) {
    for (int i = 0; i < nSLDLayers; i++)
      freeLayer(&pasSLDLayers[i]);
    msFree(pasSLDLayers);
  }

  if (map->debug == MS_DEBUGLEVEL_VVV) {
    char *tmpfilename = msTmpFile(map, map->mappath, NULL, "_sld.map");
    if (tmpfilename == NULL) {
      tmpfilename = msTmpFile(map, NULL, NULL, "_sld.map");
    }
    if (tmpfilename) {
      msSaveMap(map, tmpfilename);
      msDebug("msApplySLD(): Map file after SLD was applied %s\n", tmpfilename);
      msFree(tmpfilename);
    }
  }
  return nStatus;

#else
  msSetError(MS_MISCERR, "OWS support is not available.", "msSLDApplySLD()");
  return (MS_FAILURE);
#endif
}

static CPLXMLNode *FindNextChild(CPLXMLNode *psNode, const char *pszChildName) {
  while (psNode) {
    if (psNode->eType == CXT_Element &&
        strcasecmp(psNode->pszValue, pszChildName) == 0) {
      return psNode;
    }
    psNode = psNode->psNext;
  }
  return NULL;
}

#define LOOP_ON_CHILD_ELEMENT(psParent_, psChild_, pszChildName_)              \
  for (psChild_ = FindNextChild(psParent_->psChild, pszChildName_);            \
       psChild_ != NULL;                                                       \
       psChild_ = FindNextChild(psChild_->psNext, pszChildName_))

/************************************************************************/
/*                              msSLDParseSLD                           */
/*                                                                      */
/*      Parse the sld document into layers : for each named layer       */
/*      there is one mapserver layer created with appropriate           */
/*      classes and styles.                                             */
/*      Returns an array of mapserver layers. The pnLayres if           */
/*      provided will indicate the size of the returned array.          */
/************************************************************************/
layerObj *msSLDParseSLD(mapObj *map, const char *psSLDXML, int *pnLayers) {
  CPLXMLNode *psRoot = NULL;
  CPLXMLNode *psSLD, *psNamedLayer;
  layerObj *pasLayers = NULL;
  int iLayer = 0;
  int nLayers = 0;

  if (map == NULL || psSLDXML == NULL || strlen(psSLDXML) == 0 ||
      (strstr(psSLDXML, "StyledLayerDescriptor") == NULL)) {
    msSetError(MS_WMSERR, "Invalid SLD document", "");
    return NULL;
  }

  psRoot = CPLParseXMLString(psSLDXML);
  if (psRoot == NULL) {
    msSetError(MS_WMSERR, "Invalid SLD document : %s", "", psSLDXML);
    return NULL;
  }

  /* strip namespaces ogc and sld and gml */
  CPLStripXMLNamespace(psRoot, "ogc", 1);
  CPLStripXMLNamespace(psRoot, "sld", 1);
  CPLStripXMLNamespace(psRoot, "gml", 1);
  CPLStripXMLNamespace(psRoot, "se", 1);

  /* -------------------------------------------------------------------- */
  /*      get the root element (StyledLayerDescriptor).                   */
  /* -------------------------------------------------------------------- */
  psSLD = CPLGetXMLNode(psRoot, "=StyledLayerDescriptor");
  if (!psSLD) {
    msSetError(MS_WMSERR, "Invalid SLD document : %s", "", psSLDXML);
    return NULL;
  }

  /* -------------------------------------------------------------------- */
  /*      Parse the named layers.                                         */
  /* -------------------------------------------------------------------- */
  LOOP_ON_CHILD_ELEMENT(psSLD, psNamedLayer, "NamedLayer") { nLayers++; }

  if (nLayers > 0)
    pasLayers = (layerObj *)malloc(sizeof(layerObj) * nLayers);
  else
    return NULL;

  LOOP_ON_CHILD_ELEMENT(psSLD, psNamedLayer, "NamedLayer") {
    CPLXMLNode *psName = CPLGetXMLNode(psNamedLayer, "Name");
    initLayer(&pasLayers[iLayer], map);

    if (psName && psName->psChild && psName->psChild->pszValue)
      pasLayers[iLayer].name = msStrdup(psName->psChild->pszValue);

    if (msSLDParseNamedLayer(psNamedLayer, &pasLayers[iLayer]) != MS_SUCCESS) {
      int i;
      for (i = 0; i <= iLayer; i++)
        freeLayer(&pasLayers[i]);
      msFree(pasLayers);
      nLayers = 0;
      pasLayers = NULL;
      break;
    }

    iLayer++;
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
void _SLDApplyRuleValues(CPLXMLNode *psRule, layerObj *psLayer,
                         int nNewClasses) {
  CPLXMLNode *psMinScale = NULL, *psMaxScale = NULL;
  CPLXMLNode *psName = NULL, *psTitle = NULL;
  double dfMinScale = 0, dfMaxScale = 0;
  char *pszName = NULL, *pszTitle = NULL;

  if (psRule && psLayer && nNewClasses > 0) {
    /* -------------------------------------------------------------------- */
    /*      parse minscale and maxscale.                                    */
    /* -------------------------------------------------------------------- */
    psMinScale = CPLGetXMLNode(psRule, "MinScaleDenominator");
    if (psMinScale && psMinScale->psChild && psMinScale->psChild->pszValue)
      dfMinScale = atof(psMinScale->psChild->pszValue);

    psMaxScale = CPLGetXMLNode(psRule, "MaxScaleDenominator");
    if (psMaxScale && psMaxScale->psChild && psMaxScale->psChild->pszValue)
      dfMaxScale = atof(psMaxScale->psChild->pszValue);

    /* -------------------------------------------------------------------- */
    /*      parse name and title.                                           */
    /* -------------------------------------------------------------------- */
    psName = CPLGetXMLNode(psRule, "Name");
    if (psName && psName->psChild && psName->psChild->pszValue)
      pszName = psName->psChild->pszValue;

    psTitle = CPLGetXMLNode(psRule, "Title");
    if (psTitle && psTitle->psChild && psTitle->psChild->pszValue)
      pszTitle = psTitle->psChild->pszValue;

    /* -------------------------------------------------------------------- */
    /*      set the scale to all the classes created by the rule.           */
    /* -------------------------------------------------------------------- */
    if (dfMinScale > 0 || dfMaxScale > 0) {
      for (int i = 0; i < nNewClasses; i++) {
        if (dfMinScale > 0)
          psLayer->_class[psLayer->numclasses - 1 - i]->minscaledenom =
              dfMinScale;
        if (dfMaxScale)
          psLayer->_class[psLayer->numclasses - 1 - i]->maxscaledenom =
              dfMaxScale;
      }
    }
    /* -------------------------------------------------------------------- */
    /*      set name and title to the classes created by the rule.          */
    /* -------------------------------------------------------------------- */
    for (int i = 0; i < nNewClasses; i++) {
      if (!psLayer->_class[psLayer->numclasses - 1 - i]->name) {
        if (pszName)
          psLayer->_class[psLayer->numclasses - 1 - i]->name =
              msStrdup(pszName);
        else if (pszTitle)
          psLayer->_class[psLayer->numclasses - 1 - i]->name =
              msStrdup(pszTitle);
        else {
          // Build a name from layer and class info
          char szTmp[256];
          snprintf(szTmp, sizeof(szTmp), "%s#%d", psLayer->name,
                   psLayer->numclasses - 1 - i);
          psLayer->_class[psLayer->numclasses - 1 - i]->name = msStrdup(szTmp);
        }
      }
    }
    if (pszTitle) {
      for (int i = 0; i < nNewClasses; i++) {
        psLayer->_class[psLayer->numclasses - 1 - i]->title =
            msStrdup(pszTitle);
      }
    }
  }
}

/************************************************************************/
/*                     msSLDGetCommonExpressionFromFilter               */
/*                                                                      */
/*      Get a common expression valid from the filter valid for the    */
/*      temporary layer.                                                */
/************************************************************************/
static char *msSLDGetCommonExpressionFromFilter(CPLXMLNode *psFilter,
                                                layerObj *psLayer) {
  char *pszExpression = NULL;
  CPLXMLNode *psTmpNextNode = NULL;
  CPLXMLNode *psTmpNode = NULL;
  FilterEncodingNode *psNode = NULL;
  char *pszTmpFilter = NULL;
  layerObj *psCurrentLayer = NULL;
  const char *pszWmsName = NULL;
  const char *key = NULL;

  /* clone the tree and set the next node to null */
  /* so we only have the Filter node */
  psTmpNode = CPLCloneXMLTree(psFilter);
  psTmpNextNode = psTmpNode->psNext;
  psTmpNode->psNext = NULL;
  pszTmpFilter = CPLSerializeXMLTree(psTmpNode);
  psTmpNode->psNext = psTmpNextNode;
  CPLDestroyXMLNode(psTmpNode);

  if (pszTmpFilter) {
    psNode = FLTParseFilterEncoding(pszTmpFilter);

    CPLFree(pszTmpFilter);
  }

  if (psNode) {
    int j;

    /*preparse the filter for possible gml aliases set on the layer's metadata:
    "gml_NA3DESC_alias" "alias_name" and filter could be
    <ogc:PropertyName>alias_name</ogc:PropertyName> #3079*/
    for (j = 0; j < psLayer->map->numlayers; j++) {
      psCurrentLayer = GET_LAYER(psLayer->map, j);

      pszWmsName =
          msOWSLookupMetadata(&(psCurrentLayer->metadata), "MO", "name");

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
      the original layer has, allowing to do parsing for
      such things as gml_attribute_type #3052*/
      while (1) {
        key = msNextKeyFromHashTable(&psCurrentLayer->metadata, key);
        if (!key)
          break;
        else
          msInsertHashTable(&psLayer->metadata, key,
                            msLookupHashTable(&psCurrentLayer->metadata, key));
      }
      FLTPreParseFilterForAliasAndGroup(psNode, psLayer->map, j, "G");
    }

    pszExpression = FLTGetCommonExpression(psNode, psLayer);
    FLTFreeFilterEncodingNode(psNode);
  }

  return pszExpression;
}

/************************************************************************/
/*                          msSLDParseUserStyle                         */
/*                                                                      */
/*      Parse UserStyle node.                                           */
/************************************************************************/

static void msSLDParseUserStyle(CPLXMLNode *psUserStyle, layerObj *psLayer) {
  CPLXMLNode *psFeatureTypeStyle;
  const char *pszUserStyleName = CPLGetXMLValue(psUserStyle, "Name", NULL);
  if (pszUserStyleName) {
    const char *pszIsDefault = CPLGetXMLValue(psUserStyle, "IsDefault", "0");
    if (EQUAL(pszIsDefault, "true") || EQUAL(pszIsDefault, "1")) {
      msFree(psLayer->classgroup);
      psLayer->classgroup = msStrdup(pszUserStyleName);
    }
  }

  LOOP_ON_CHILD_ELEMENT(psUserStyle, psFeatureTypeStyle, "FeatureTypeStyle") {
    CPLXMLNode *psRule;

    /* -------------------------------------------------------------------- */
    /*      Parse rules with no Else filter.                                */
    /* -------------------------------------------------------------------- */
    LOOP_ON_CHILD_ELEMENT(psFeatureTypeStyle, psRule, "Rule") {
      CPLXMLNode *psFilter = NULL;
      CPLXMLNode *psElseFilter = NULL;
      int nNewClasses = 0, nClassBeforeFilter = 0, nClassAfterFilter = 0;
      int nClassAfterRule = 0, nClassBeforeRule = 0;

      /* used for scale setting */
      nClassBeforeRule = psLayer->numclasses;

      psElseFilter = CPLGetXMLNode(psRule, "ElseFilter");
      nClassBeforeFilter = psLayer->numclasses;
      if (psElseFilter == NULL)
        msSLDParseRule(psRule, psLayer, pszUserStyleName);
      nClassAfterFilter = psLayer->numclasses;

      /* -------------------------------------------------------------------- */
      /*      Parse the filter and apply it to the latest class created by    */
      /*      the rule.                                                       */
      /*      NOTE : Spatial Filter is not supported.                         */
      /* -------------------------------------------------------------------- */
      psFilter = CPLGetXMLNode(psRule, "Filter");
      if (psFilter && psFilter->psChild && psFilter->psChild->pszValue) {
        char *pszExpression =
            msSLDGetCommonExpressionFromFilter(psFilter, psLayer);
        if (pszExpression) {
          int i;
          nNewClasses = nClassAfterFilter - nClassBeforeFilter;
          for (i = 0; i < nNewClasses; i++) {
            expressionObj *exp =
                &(psLayer->_class[psLayer->numclasses - 1 - i]->expression);
            msFreeExpression(exp);
            msInitExpression(exp);
            exp->string = msStrdup(pszExpression);
            exp->type = MS_EXPRESSION;
          }
          msFree(pszExpression);
          pszExpression = NULL;
        }
      }
      nClassAfterRule = psLayer->numclasses;
      nNewClasses = nClassAfterRule - nClassBeforeRule;

      /* apply scale and title to newly created classes */
      _SLDApplyRuleValues(psRule, psLayer, nNewClasses);

      /* TODO : parse legendgraphic */
    }
    /* -------------------------------------------------------------------- */
    /*      First parse rules with the else filter. These rules will        */
    /*      create the classes that are placed at the end of class          */
    /*      list. (See how classes are applied to layers in function        */
    /*      msSLDApplySLD).                                                 */
    /* -------------------------------------------------------------------- */
    LOOP_ON_CHILD_ELEMENT(psFeatureTypeStyle, psRule, "Rule") {
      CPLXMLNode *psElseFilter = CPLGetXMLNode(psRule, "ElseFilter");
      if (psElseFilter) {
        msSLDParseRule(psRule, psLayer, pszUserStyleName);
        _SLDApplyRuleValues(psRule, psLayer, 1);
        psLayer->_class[psLayer->numclasses - 1]->isfallback = TRUE;
      }
    }
  }
}

/************************************************************************/
/*                           msSLDParseNamedLayer                       */
/*                                                                      */
/*      Parse NamedLayer root.                                          */
/************************************************************************/
int msSLDParseNamedLayer(CPLXMLNode *psRoot, layerObj *psLayer) {
  CPLXMLNode *psLayerFeatureConstraints = NULL;

  if (!psRoot || !psLayer)
    return MS_FAILURE;

  if (CPLGetXMLNode(psRoot, "UserStyle")) {
    CPLXMLNode *psUserStyle;
    LOOP_ON_CHILD_ELEMENT(psRoot, psUserStyle, "UserStyle") {
      msSLDParseUserStyle(psUserStyle, psLayer);
    }
  }
  /* check for Named styles*/
  else {
    CPLXMLNode *psNamedStyle = CPLGetXMLNode(psRoot, "NamedStyle");
    if (psNamedStyle) {
      CPLXMLNode *psSLDName = CPLGetXMLNode(psNamedStyle, "Name");
      if (psSLDName && psSLDName->psChild && psSLDName->psChild->pszValue) {
        msFree(psLayer->classgroup);
        psLayer->classgroup = msStrdup(psSLDName->psChild->pszValue);
      }
    }
  }

  /* Deal with LayerFeatureConstraints */
  psLayerFeatureConstraints = CPLGetXMLNode(psRoot, "LayerFeatureConstraints");
  if (psLayerFeatureConstraints != NULL) {
    CPLXMLNode *psIter = psLayerFeatureConstraints->psChild;
    CPLXMLNode *psFeatureTypeConstraint = NULL;
    for (; psIter != NULL; psIter = psIter->psNext) {
      if (psIter->eType == CXT_Element &&
          strcmp(psIter->pszValue, "FeatureTypeConstraint") == 0) {
        if (psFeatureTypeConstraint == NULL) {
          psFeatureTypeConstraint = psIter;
        } else {
          msSetError(MS_WMSERR,
                     "Only one single FeatureTypeConstraint element "
                     "per LayerFeatureConstraints is supported",
                     "");
          return MS_FAILURE;
        }
      }
    }
    if (psFeatureTypeConstraint != NULL) {
      CPLXMLNode *psFilter;
      if (CPLGetXMLNode(psFeatureTypeConstraint, "FeatureTypeName") != NULL) {
        msSetError(MS_WMSERR,
                   "FeatureTypeName element is not "
                   "supported in FeatureTypeConstraint",
                   "");
        return MS_FAILURE;
      }
      if (CPLGetXMLNode(psFeatureTypeConstraint, "Extent") != NULL) {
        msSetError(MS_WMSERR,
                   "Extent element is not "
                   "supported in FeatureTypeConstraint",
                   "");
        return MS_FAILURE;
      }
      psFilter = CPLGetXMLNode(psFeatureTypeConstraint, "Filter");
      if (psFilter && psFilter->psChild && psFilter->psChild->pszValue) {
        char *pszExpression =
            msSLDGetCommonExpressionFromFilter(psFilter, psLayer);
        if (pszExpression) {
          msFreeExpression(&psLayer->filter);
          msInitExpression(&psLayer->filter);
          psLayer->filter.string = pszExpression;
          psLayer->filter.type = MS_EXPRESSION;
        }
      }
    }
  }

  return MS_SUCCESS;
}

/************************************************************************/
/*                      msSLDParseRule()                                */
/*                                                                      */
/*      Parse a Rule node into classes for a specific layer.            */
/************************************************************************/
int msSLDParseRule(CPLXMLNode *psRoot, layerObj *psLayer,
                   const char *pszUserStyleName) {
  CPLXMLNode *psLineSymbolizer = NULL;
  CPLXMLNode *psPolygonSymbolizer = NULL;
  CPLXMLNode *psPointSymbolizer = NULL;
  CPLXMLNode *psTextSymbolizer = NULL;
  CPLXMLNode *psRasterSymbolizer = NULL;

  int nSymbolizer = 0;

  if (!psRoot || !psLayer)
    return MS_FAILURE;

  /* TODO : parse name of the rule */
  /* ==================================================================== */
  /*      For each rule a new class is created. If there are more than    */
  /*      one symbolizer, a style is added in the same class.             */
  /* ==================================================================== */

  /* Raster symbolizer */
  LOOP_ON_CHILD_ELEMENT(psRoot, psRasterSymbolizer, "RasterSymbolizer") {
    msSLDParseRasterSymbolizer(psRasterSymbolizer, psLayer, pszUserStyleName);
    /* cppcheck-suppress knownConditionTrueFalse */
    if (nSymbolizer == 0) {
      psLayer->type = MS_LAYER_RASTER;
    }
  }

  /* Polygon symbolizer */
  LOOP_ON_CHILD_ELEMENT(psRoot, psPolygonSymbolizer, "PolygonSymbolizer") {
    /* cppcheck-suppress knownConditionTrueFalse */
    const int bNewClass = (nSymbolizer == 0);
    msSLDParsePolygonSymbolizer(psPolygonSymbolizer, psLayer, bNewClass,
                                pszUserStyleName);
    psLayer->type = MS_LAYER_POLYGON;
    nSymbolizer++;
  }

  /* line symbolizer */
  LOOP_ON_CHILD_ELEMENT(psRoot, psLineSymbolizer, "LineSymbolizer") {
    const int bNewClass = (nSymbolizer == 0);
    msSLDParseLineSymbolizer(psLineSymbolizer, psLayer, bNewClass,
                             pszUserStyleName);
    if (bNewClass) {
      psLayer->type = MS_LAYER_LINE;
    }
    if (psLayer->type == MS_LAYER_POLYGON) {
      const int nClassId = psLayer->numclasses - 1;
      if (nClassId >= 0) {
        const int nStyleId = psLayer->_class[nClassId]->numstyles - 1;
        if (nStyleId >= 0) {
          styleObj *psStyle = psLayer->_class[nClassId]->styles[nStyleId];
          psStyle->outlinecolor = psStyle->color;
          MS_INIT_COLOR(psStyle->color, -1, -1, -1, 255);
          MS_COPYSTRING(
              psStyle->exprBindings[MS_STYLE_BINDING_OUTLINECOLOR].string,
              psStyle->exprBindings[MS_STYLE_BINDING_COLOR].string);
          psStyle->exprBindings[MS_STYLE_BINDING_OUTLINECOLOR].type =
              psStyle->exprBindings[MS_STYLE_BINDING_COLOR].type;
          msFreeExpression(&(psStyle->exprBindings[MS_STYLE_BINDING_COLOR]));
          msInitExpression(&(psStyle->exprBindings[MS_STYLE_BINDING_COLOR]));
        }
      }
    }
    nSymbolizer++;
  }

  /* Point Symbolizer */
  LOOP_ON_CHILD_ELEMENT(psRoot, psPointSymbolizer, "PointSymbolizer") {
    const int bNewClass = (nSymbolizer == 0);
    msSLDParsePointSymbolizer(psPointSymbolizer, psLayer, bNewClass,
                              pszUserStyleName);
    if (bNewClass) {
      psLayer->type = MS_LAYER_POINT;
    }
    if (psLayer->type == MS_LAYER_POLYGON || psLayer->type == MS_LAYER_LINE ||
        psLayer->type == MS_LAYER_RASTER) {
      const int nClassId = psLayer->numclasses - 1;
      if (nClassId >= 0) {
        const int nStyleId = psLayer->_class[nClassId]->numstyles - 1;
        if (nStyleId >= 0) {
          styleObj *psStyle = psLayer->_class[nClassId]->styles[nStyleId];
          msStyleSetGeomTransform(psStyle, "centroid");
        }
      }
    }
    nSymbolizer++;
  }

  /* Text symbolizer */
  /* ==================================================================== */
  /*      For text symbolizer, here is how it is translated into          */
  /*      mapserver classes :                                             */
  /*        - If there are other symbolizers(line, polygon, symbol),      */
  /*      the label object created will be created in the same class      */
  /*      (the last class) as the  symbolizer. This allows to have for    */
  /*      example of point layer with labels.                             */
  /*        - If there are no other symbolizers, a new class will be      */
  /*      created to contain the label object.                            */
  /* ==================================================================== */
  LOOP_ON_CHILD_ELEMENT(psRoot, psTextSymbolizer, "TextSymbolizer") {
    if (nSymbolizer == 0)
      psLayer->type = MS_LAYER_POINT;
    msSLDParseTextSymbolizer(psTextSymbolizer, psLayer, nSymbolizer > 0,
                             pszUserStyleName);
  }

  return MS_SUCCESS;
}

/************************************************************************/
/*                            getClassId()                              */
/************************************************************************/

static int getClassId(layerObj *psLayer, int bNewClass,
                      const char *pszUserStyleName) {
  int nClassId;
  if (bNewClass || psLayer->numclasses <= 0) {
    if (msGrowLayerClasses(psLayer) == NULL)
      return -1;
    initClass(psLayer->_class[psLayer->numclasses]);
    nClassId = psLayer->numclasses;
    if (pszUserStyleName)
      psLayer->_class[nClassId]->group = msStrdup(pszUserStyleName);
    psLayer->numclasses++;
  } else {
    nClassId = psLayer->numclasses - 1;
  }
  return nClassId;
}

/************************************************************************/
/*                      msSLDParseUomAttribute()                        */
/************************************************************************/

int msSLDParseUomAttribute(CPLXMLNode *node, enum MS_UNITS *sizeunits) {
  const struct {
    enum MS_UNITS unit;
    const char *const values[10];
  } known_uoms[] = {
      {MS_INCHES, {"inch", "inches", NULL}},
      {MS_FEET,
       {"foot", "feet", "http://www.opengeospatial.org/se/units/foot", NULL}},
      {MS_MILES, {"mile", "miles", NULL}},
      {MS_METERS,
       {"meter", "meters", "metre", "metres",
        "http://www.opengeospatial.org/se/units/metre", NULL}},
      {MS_KILOMETERS,
       {"kilometer", "kilometers", "kilometre", "kilometres", NULL}},
      {MS_DD, {"dd", NULL}},
      {MS_PIXELS,
       {"pixel", "pixels", "px", "http://www.opengeospatial.org/se/units/pixel",
        NULL}},
      {MS_PERCENTAGES,
       {"percent", "percents", "percentage", "percentages", NULL}},
      {MS_NAUTICALMILES,
       {"nauticalmile", "nauticalmiles", "nautical_mile", "nautical_miles",
        NULL}},
      {MS_INCHES, {NULL}}};

  const char *uom = CPLGetXMLValue(node, "uom", NULL);
  if (uom) {
    for (int i = 0; known_uoms[i].values[0]; i++)
      for (int j = 0; known_uoms[i].values[j]; j++)
        if (strcmp(uom, known_uoms[i].values[j]) == 0) {
          // Match found
          *sizeunits = known_uoms[i].unit;
          return MS_SUCCESS;
        }
    // No match was found
    return MS_FAILURE;
  }
  // No uom was found
  *sizeunits = MS_PIXELS;
  return MS_SUCCESS;
}

/************************************************************************/
/*                 msSLDParseLineSymbolizer()                           */
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
                             int bNewClass, const char *pszUserStyleName) {
  CPLXMLNode *psStroke = NULL, *psOffset = NULL;

  if (!psRoot || !psLayer)
    return MS_FAILURE;

  // Get uom if any, defaults to MS_PIXELS
  enum MS_UNITS sizeunits = MS_PIXELS;
  if (msSLDParseUomAttribute(psRoot, &sizeunits) != MS_SUCCESS) {
    msSetError(MS_WMSERR, "Invalid uom attribute value.",
               "msSLDParsePolygonSymbolizer()");
    return MS_FAILURE;
  }

  psStroke = CPLGetXMLNode(psRoot, "Stroke");
  if (psStroke) {
    int nClassId = getClassId(psLayer, bNewClass, pszUserStyleName);
    if (nClassId < 0)
      return MS_FAILURE;

    const int iStyle = psLayer->_class[nClassId]->numstyles;
    msMaybeAllocateClassStyle(psLayer->_class[nClassId], iStyle);
    psLayer->_class[nClassId]->styles[iStyle]->sizeunits = sizeunits;

    msSLDParseStroke(psStroke, psLayer->_class[nClassId]->styles[iStyle],
                     psLayer->map, 0);

    /*parse PerpendicularOffset SLD 1.1.10*/
    psOffset = CPLGetXMLNode(psRoot, "PerpendicularOffset");
    if (psOffset && psOffset->psChild && psOffset->psChild->pszValue) {
      psLayer->_class[nClassId]->styles[iStyle]->offsetx =
          atoi(psOffset->psChild->pszValue);
      psLayer->_class[nClassId]->styles[iStyle]->offsety =
          MS_STYLE_SINGLE_SIDED_OFFSET;
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
int msSLDParseStroke(CPLXMLNode *psStroke, styleObj *psStyle, mapObj *map,
                     int iColorParam) {
  CPLXMLNode *psCssParam = NULL, *psGraphicFill = NULL;
  char *psStrkName = NULL;
  char *pszDashValue = NULL;

  if (!psStroke || !psStyle)
    return MS_FAILURE;

  /* parse css parameters */
  psCssParam = CPLGetXMLNode(psStroke, "CssParameter");
  /*sld 1.1 used SvgParameter*/
  if (psCssParam == NULL)
    psCssParam = CPLGetXMLNode(psStroke, "SvgParameter");

  while (psCssParam && psCssParam->pszValue &&
         (strcasecmp(psCssParam->pszValue, "CssParameter") == 0 ||
          strcasecmp(psCssParam->pszValue, "SvgParameter") == 0)) {
    psStrkName = (char *)CPLGetXMLValue(psCssParam, "name", NULL);

    if (psStrkName) {
      if (strcasecmp(psStrkName, "stroke") == 0) {
        if (psCssParam->psChild && psCssParam->psChild->psNext) {
          switch (iColorParam) {
          case 0:
            msSLDParseOgcExpression(psCssParam->psChild->psNext, psStyle,
                                    MS_STYLE_BINDING_COLOR, MS_OBJ_STYLE);
            break;
          case 1:
            msSLDParseOgcExpression(psCssParam->psChild->psNext, psStyle,
                                    MS_STYLE_BINDING_OUTLINECOLOR,
                                    MS_OBJ_STYLE);
            break;
          }
        }
      } else if (strcasecmp(psStrkName, "stroke-width") == 0) {
        if (psCssParam->psChild && psCssParam->psChild->psNext) {
          msSLDParseOgcExpression(psCssParam->psChild->psNext, psStyle,
                                  MS_STYLE_BINDING_WIDTH, MS_OBJ_STYLE);
        }
      } else if (strcasecmp(psStrkName, "stroke-dasharray") == 0) {
        if (psCssParam->psChild && psCssParam->psChild->psNext &&
            psCssParam->psChild->psNext->pszValue) {
          int nDash = 0, i;
          char **aszValues = NULL;
          int nMaxDash;
          if (pszDashValue)
            free(pszDashValue); /* free previous if multiple stroke-dasharray
                                   attributes were found */
          pszDashValue = msStrdup(psCssParam->psChild->psNext->pszValue);
          aszValues = msStringSplit(pszDashValue, ' ', &nDash);
          if (nDash > 0) {
            nMaxDash = nDash;
            if (nDash > MS_MAXPATTERNLENGTH)
              nMaxDash = MS_MAXPATTERNLENGTH;

            psStyle->patternlength = nMaxDash;
            for (i = 0; i < nMaxDash; i++)
              psStyle->pattern[i] = atof(aszValues[i]);

            psStyle->linecap = MS_CJC_BUTT;
          }
          msFreeCharArray(aszValues, nDash);
        }
      } else if (strcasecmp(psStrkName, "stroke-opacity") == 0) {
        if (psCssParam->psChild && psCssParam->psChild->psNext) {
          msSLDParseOgcExpression(psCssParam->psChild->psNext, psStyle,
                                  MS_STYLE_BINDING_OPACITY, MS_OBJ_STYLE);
        }
      }
    }
    psCssParam = psCssParam->psNext;
  }

  /* parse graphic fill or stroke */
  /* graphic fill and graphic stroke pare parsed the same way :  */
  /* TODO : It seems inconsistent to me since the only difference */
  /* between them seems to be fill (fill) or not fill (stroke). And */
  /* then again the fill parameter can be used inside both elements. */
  psGraphicFill = CPLGetXMLNode(psStroke, "GraphicFill");
  if (psGraphicFill)
    msSLDParseGraphicFillOrStroke(psGraphicFill, pszDashValue, psStyle, map);
  psGraphicFill = CPLGetXMLNode(psStroke, "GraphicStroke");
  if (psGraphicFill)
    msSLDParseGraphicFillOrStroke(psGraphicFill, pszDashValue, psStyle, map);

  if (pszDashValue)
    free(pszDashValue);

  return MS_SUCCESS;
}

/************************************************************************/
/*  int msSLDParseOgcExpression(CPLXMLNode *psRoot, styleObj *psStyle,  */
/*                              enum MS_STYLE_BINDING_ENUM binding)     */
/*                                                                      */
/*              Parse an OGC expression in a <SvgParameter>             */
/************************************************************************/
int msSLDParseOgcExpression(CPLXMLNode *psRoot, void *psObj, int binding,
                            enum objType objtype) {
  int status = MS_FAILURE;
  const char *ops = "Add+Sub-Mul*Div/";
  styleObj *psStyle = static_cast<styleObj *>(psObj);
  labelObj *psLabel = static_cast<labelObj *>(psObj);
  int lbinding;
  expressionObj *exprBindings;
  int *nexprbindings;
  enum { MS_STYLE_BASE = 0, MS_LABEL_BASE = 100 };

  switch (objtype) {
  case MS_OBJ_STYLE:
    lbinding = binding + MS_STYLE_BASE;
    exprBindings = psStyle->exprBindings;
    nexprbindings = &psStyle->nexprbindings;
    break;
  case MS_OBJ_LABEL:
    lbinding = binding + MS_LABEL_BASE;
    exprBindings = psLabel->exprBindings;
    nexprbindings = &psLabel->nexprbindings;
    break;
  default:
    return MS_FAILURE;
    break;
  }

  switch (psRoot->eType) {
  case CXT_Text:
    // Parse a raw value
    {
      msStringBuffer *literal = msStringBufferAlloc();
      msStringBufferAppend(literal, "(");
      msStringBufferAppend(literal, psRoot->pszValue);
      msStringBufferAppend(literal, ")");
      msFreeExpression(&(exprBindings[binding]));
      msInitExpression(&(exprBindings[binding]));
      exprBindings[binding].string =
          msStringBufferReleaseStringAndFree(literal);
      exprBindings[binding].type = MS_STRING;
    }
    switch (lbinding) {
    case MS_STYLE_BASE + MS_STYLE_BINDING_OFFSET_X:
      psStyle->offsetx = atoi(psRoot->pszValue);
      status = MS_SUCCESS;
      break;
    case MS_STYLE_BASE + MS_STYLE_BINDING_OFFSET_Y:
      psStyle->offsety = atoi(psRoot->pszValue);
      status = MS_SUCCESS;
      break;
    case MS_STYLE_BASE + MS_STYLE_BINDING_ANGLE:
      psStyle->angle = atof(psRoot->pszValue);
      status = MS_SUCCESS;
      break;
    case MS_STYLE_BASE + MS_STYLE_BINDING_SIZE:
      psStyle->size = atof(psRoot->pszValue);
      status = MS_SUCCESS;
      break;
    case MS_STYLE_BASE + MS_STYLE_BINDING_WIDTH:
      psStyle->width = atof(psRoot->pszValue);
      status = MS_SUCCESS;
      break;
    case MS_STYLE_BASE + MS_STYLE_BINDING_OPACITY:
      psStyle->opacity = atof(psRoot->pszValue) * 100;
      status = MS_SUCCESS;
      // Apply opacity as the alpha channel color(s)
      if (psStyle->opacity < 100) {
        int alpha = MS_NINT(psStyle->opacity * 2.55);
        psStyle->color.alpha = alpha;
        psStyle->outlinecolor.alpha = alpha;
        psStyle->mincolor.alpha = alpha;
        psStyle->maxcolor.alpha = alpha;
      }
      break;
    case MS_STYLE_BASE + MS_STYLE_BINDING_COLOR:
      if (strlen(psRoot->pszValue) == 7 && psRoot->pszValue[0] == '#') {
        psStyle->color.red = msHexToInt(psRoot->pszValue + 1);
        psStyle->color.green = msHexToInt(psRoot->pszValue + 3);
        psStyle->color.blue = msHexToInt(psRoot->pszValue + 5);
        status = MS_SUCCESS;
      }
      break;
    case MS_STYLE_BASE + MS_STYLE_BINDING_OUTLINECOLOR:
      if (strlen(psRoot->pszValue) == 7 && psRoot->pszValue[0] == '#') {
        psStyle->outlinecolor.red = msHexToInt(psRoot->pszValue + 1);
        psStyle->outlinecolor.green = msHexToInt(psRoot->pszValue + 3);
        psStyle->outlinecolor.blue = msHexToInt(psRoot->pszValue + 5);
        status = MS_SUCCESS;
      }
      break;

    case MS_LABEL_BASE + MS_LABEL_BINDING_SIZE:
      psLabel->size = atof(psRoot->pszValue);
      if (psLabel->size <= 0.0) {
        psLabel->size = 10.0;
      }
      status = MS_SUCCESS;
      break;
    case MS_LABEL_BASE + MS_LABEL_BINDING_ANGLE:
      psLabel->angle = atof(psRoot->pszValue);
      status = MS_SUCCESS;
      break;
    case MS_LABEL_BASE + MS_LABEL_BINDING_COLOR:
      if (strlen(psRoot->pszValue) == 7 && psRoot->pszValue[0] == '#') {
        psLabel->color.red = msHexToInt(psRoot->pszValue + 1);
        psLabel->color.green = msHexToInt(psRoot->pszValue + 3);
        psLabel->color.blue = msHexToInt(psRoot->pszValue + 5);
        status = MS_SUCCESS;
      }
      break;
    case MS_LABEL_BASE + MS_LABEL_BINDING_OUTLINECOLOR:
      if (strlen(psRoot->pszValue) == 7 && psRoot->pszValue[0] == '#') {
        psLabel->outlinecolor.red = msHexToInt(psRoot->pszValue + 1);
        psLabel->outlinecolor.green = msHexToInt(psRoot->pszValue + 3);
        psLabel->outlinecolor.blue = msHexToInt(psRoot->pszValue + 5);
        status = MS_SUCCESS;
      }
      break;
    default:
      break;
    }
    break;
  case CXT_Element:
    if (strcasecmp(psRoot->pszValue, "Literal") == 0 && psRoot->psChild) {
      // Parse a <ogc:Literal> element
      status =
          msSLDParseOgcExpression(psRoot->psChild, psObj, binding, objtype);
    } else if (strcasecmp(psRoot->pszValue, "PropertyName") == 0 &&
               psRoot->psChild) {
      // Parse a <ogc:PropertyName> element
      msStringBuffer *property = msStringBufferAlloc();
      const char *strDelim = "";

      switch (lbinding) {
      case MS_STYLE_BASE + MS_STYLE_BINDING_COLOR:
      case MS_STYLE_BASE + MS_STYLE_BINDING_OUTLINECOLOR:
      case MS_LABEL_BASE + MS_LABEL_BINDING_COLOR:
      case MS_LABEL_BASE + MS_LABEL_BINDING_OUTLINECOLOR:
        strDelim = "\"";
        /* FALLTHROUGH */
      default:
        msStringBufferAppend(property, strDelim);
        msStringBufferAppend(property, "[");
        msStringBufferAppend(property, psRoot->psChild->pszValue);
        msStringBufferAppend(property, "]");
        msStringBufferAppend(property, strDelim);
        msInitExpression(&(exprBindings[binding]));
        exprBindings[binding].string =
            msStringBufferReleaseStringAndFree(property);
        exprBindings[binding].type = MS_EXPRESSION;
        (*nexprbindings)++;
        break;
      }
      status = MS_SUCCESS;
    } else if (strcasecmp(psRoot->pszValue, "Function") == 0 &&
               psRoot->psChild && CPLGetXMLValue(psRoot, "name", NULL) &&
               psRoot->psChild->psNext) {
      // Parse a <ogc:Function> element
      msStringBuffer *function = msStringBufferAlloc();

      // Parse function name
      const char *funcname = CPLGetXMLValue(psRoot, "name", NULL);
      msStringBufferAppend(function, funcname);
      msStringBufferAppend(function, "(");
      msInitExpression(&(exprBindings[binding]));

      // Parse arguments
      const char *sep = "";
      for (CPLXMLNode *argument = psRoot->psChild->psNext; argument;
           argument = argument->psNext) {
        status = msSLDParseOgcExpression(argument, psObj, binding, objtype);
        if (status != MS_SUCCESS)
          break;
        msStringBufferAppend(function, sep);
        msStringBufferAppend(function, exprBindings[binding].string);
        msFree(exprBindings[binding].string);
        msInitExpression(&(exprBindings[binding]));
        sep = ",";
      }
      msStringBufferAppend(function, ")");
      exprBindings[binding].string =
          msStringBufferReleaseStringAndFree(function);
      exprBindings[binding].type = MS_EXPRESSION;
      (*nexprbindings)++;
      status = MS_SUCCESS;
    } else if (strstr(ops, psRoot->pszValue) && psRoot->psChild &&
               psRoot->psChild->psNext) {
      // Parse an arithmetic element <ogc:Add>, <ogc:Sub>, <ogc:Mul>, <ogc:Div>
      const char op[2] = {*(strstr(ops, psRoot->pszValue) + 3), '\0'};
      msStringBuffer *expression = msStringBufferAlloc();

      // Parse first operand
      msStringBufferAppend(expression, "(");
      msInitExpression(&(exprBindings[binding]));
      status =
          msSLDParseOgcExpression(psRoot->psChild, psObj, binding, objtype);

      // Parse second operand
      if (status == MS_SUCCESS) {
        msStringBufferAppend(expression, exprBindings[binding].string);
        msStringBufferAppend(expression, op);
        msFree(exprBindings[binding].string);
        msInitExpression(&(exprBindings[binding]));
        status = msSLDParseOgcExpression(psRoot->psChild->psNext, psObj,
                                         binding, objtype);
        if (status == MS_SUCCESS) {
          msStringBufferAppend(expression, exprBindings[binding].string);
          msStringBufferAppend(expression, ")");
          msFree(exprBindings[binding].string);
          exprBindings[binding].string =
              msStringBufferReleaseStringAndFree(expression);
          expression = NULL;
          exprBindings[binding].type = MS_EXPRESSION;
          (*nexprbindings)++;
        }
      }
      if (expression != NULL) {
        msStringBufferFree(expression);
        msInitExpression(&(exprBindings[binding]));
      }
    }
    break;
  default:
    break;
  }

  return status;
}

/************************************************************************/
/*                      msSLDParsePolygonSymbolizer()                   */
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
/*      fill-opacity instead of stroke-opacity. None of the other
 * CssParameters*/
/*      in Stroke are available for filling and the default value for the fill
 * color in this context is 50% gray (value #808080).*/
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
/*      The default if neither an ExternalGraphic nor a Mark is specified is to
 * use the default*/
/*      mark of a square with a 50%-gray fill and a black outline, with a size
 * of 6 pixels.*/
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
/*      The WellKnownName element gives the well-known name of the shape of the
 * mark.*/
/*      Allowed values include at least square, circle, triangle, star, cross,*/
/*      and x, though map servers may draw a different symbol instead if they
 * don't have a*/
/*      shape for all of these. The default WellKnownName is square. Renderings
 * of these*/
/*      marks may be made solid or hollow depending on Fill and Stroke
 * elements.*/
/*                                                                      */
/************************************************************************/
int msSLDParsePolygonSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer,
                                int bNewClass, const char *pszUserStyleName) {
  CPLXMLNode *psFill, *psStroke;
  CPLXMLNode *psDisplacement = NULL, *psDisplacementX = NULL,
             *psDisplacementY = NULL;
  int nOffsetX = -1, nOffsetY = -1;

  if (!psRoot || !psLayer)
    return MS_FAILURE;

  // Get uom if any, defaults to MS_PIXELS
  enum MS_UNITS sizeunits = MS_PIXELS;
  if (msSLDParseUomAttribute(psRoot, &sizeunits) != MS_SUCCESS) {
    msSetError(MS_WMSERR, "Invalid uom attribute value.",
               "msSLDParsePolygonSymbolizer()");
    return MS_FAILURE;
  }

  /*parse displacement for SLD 1.1.0*/
  psDisplacement = CPLGetXMLNode(psRoot, "Displacement");
  if (psDisplacement) {
    psDisplacementX = CPLGetXMLNode(psDisplacement, "DisplacementX");
    psDisplacementY = CPLGetXMLNode(psDisplacement, "DisplacementY");
    /* psCssParam->psChild->psNext->pszValue) */
    if (psDisplacementX && psDisplacementX->psChild &&
        psDisplacementX->psChild->pszValue && psDisplacementY &&
        psDisplacementY->psChild && psDisplacementY->psChild->pszValue) {
      nOffsetX = atoi(psDisplacementX->psChild->pszValue);
      nOffsetY = atoi(psDisplacementY->psChild->pszValue);
    }
  }

  psFill = CPLGetXMLNode(psRoot, "Fill");
  if (psFill) {
    const int nClassId = getClassId(psLayer, bNewClass, pszUserStyleName);
    if (nClassId < 0)
      return MS_FAILURE;

    const int iStyle = psLayer->_class[nClassId]->numstyles;
    msMaybeAllocateClassStyle(psLayer->_class[nClassId], iStyle);
    psLayer->_class[nClassId]->styles[iStyle]->sizeunits = sizeunits;

    msSLDParsePolygonFill(psFill, psLayer->_class[nClassId]->styles[iStyle],
                          psLayer->map);

    if (nOffsetX > 0 && nOffsetY > 0) {
      psLayer->_class[nClassId]->styles[iStyle]->offsetx = nOffsetX;
      psLayer->_class[nClassId]->styles[iStyle]->offsety = nOffsetY;
    }
  }
  /* stroke which corresponds to the outline in mapserver */
  /* is drawn after the fill */
  psStroke = CPLGetXMLNode(psRoot, "Stroke");
  if (psStroke) {
    /* -------------------------------------------------------------------- */
    /*      there was a fill so add a style to the last class created       */
    /*      by the fill                                                     */
    /* -------------------------------------------------------------------- */
    int nClassId;
    int iStyle;
    if (psFill && psLayer->numclasses > 0) {
      nClassId = psLayer->numclasses - 1;
      iStyle = psLayer->_class[nClassId]->numstyles;
      msMaybeAllocateClassStyle(psLayer->_class[nClassId], iStyle);
      psLayer->_class[nClassId]->styles[iStyle]->sizeunits = sizeunits;
    } else {
      nClassId = getClassId(psLayer, bNewClass, pszUserStyleName);
      if (nClassId < 0)
        return MS_FAILURE;

      iStyle = psLayer->_class[nClassId]->numstyles;
      msMaybeAllocateClassStyle(psLayer->_class[nClassId], iStyle);
      psLayer->_class[nClassId]->styles[iStyle]->sizeunits = sizeunits;
    }
    msSLDParseStroke(psStroke, psLayer->_class[nClassId]->styles[iStyle],
                     psLayer->map, 1);

    if (nOffsetX > 0 && nOffsetY > 0) {
      psLayer->_class[nClassId]->styles[iStyle]->offsetx = nOffsetX;
      psLayer->_class[nClassId]->styles[iStyle]->offsety = nOffsetY;
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
int msSLDParsePolygonFill(CPLXMLNode *psFill, styleObj *psStyle, mapObj *map) {
  CPLXMLNode *psCssParam, *psGraphicFill;
  char *psFillName = NULL;

  if (!psFill || !psStyle || !map)
    return MS_FAILURE;

  /* sets the default fill color defined in the spec #808080 */
  psStyle->color.red = 128;
  psStyle->color.green = 128;
  psStyle->color.blue = 128;

  psCssParam = CPLGetXMLNode(psFill, "CssParameter");
  /*sld 1.1 used SvgParameter*/
  if (psCssParam == NULL)
    psCssParam = CPLGetXMLNode(psFill, "SvgParameter");

  while (psCssParam && psCssParam->pszValue &&
         (strcasecmp(psCssParam->pszValue, "CssParameter") == 0 ||
          strcasecmp(psCssParam->pszValue, "SvgParameter") == 0)) {
    psFillName = (char *)CPLGetXMLValue(psCssParam, "name", NULL);
    if (psFillName) {
      if (strcasecmp(psFillName, "fill") == 0) {
        if (psCssParam->psChild && psCssParam->psChild->psNext) {
          msSLDParseOgcExpression(psCssParam->psChild->psNext, psStyle,
                                  MS_STYLE_BINDING_COLOR, MS_OBJ_STYLE);
        }
      } else if (strcasecmp(psFillName, "fill-opacity") == 0) {
        if (psCssParam->psChild && psCssParam->psChild->psNext) {
          msSLDParseOgcExpression(psCssParam->psChild->psNext, psStyle,
                                  MS_STYLE_BINDING_OPACITY, MS_OBJ_STYLE);
        }
      }
    }
    psCssParam = psCssParam->psNext;
  }

  /* graphic fill and graphic stroke pare parsed the same way :  */
  /* TODO : It seems inconsistent to me since the only diffrence */
  /* between them seems to be fill (fill) or not fill (stroke). And */
  /* then again the fill parameter can be used inside both elements. */
  psGraphicFill = CPLGetXMLNode(psFill, "GraphicFill");
  if (psGraphicFill)
    msSLDParseGraphicFillOrStroke(psGraphicFill, NULL, psStyle, map);
  psGraphicFill = CPLGetXMLNode(psFill, "GraphicStroke");
  if (psGraphicFill)
    msSLDParseGraphicFillOrStroke(psGraphicFill, NULL, psStyle, map);

  return MS_SUCCESS;
}

/************************************************************************/
/*                      msSLDParseGraphicFillOrStroke                   */
/*                                                                      */
/*      Parse the GraphicFill Or GraphicStroke node : look for a        */
/*      Marker symbol and set the style for that symbol.                */
/************************************************************************/
int msSLDParseGraphicFillOrStroke(CPLXMLNode *psRoot, char *pszDashValue_unused,
                                  styleObj *psStyle, mapObj *map) {
  (void)pszDashValue_unused;
  CPLXMLNode *psCssParam, *psGraphic, *psExternalGraphic, *psMark, *psSize,
      *psGap, *psInitialGap;
  CPLXMLNode *psWellKnownName, *psStroke, *psFill;
  CPLXMLNode *psDisplacement = NULL, *psDisplacementX = NULL,
             *psDisplacementY = NULL;
  CPLXMLNode *psOpacity = NULL, *psRotation = NULL;
  char *psName = NULL, *psValue = NULL;
  char *pszSymbolName = NULL;
  int bFilled = 0;

  if (!psRoot || !psStyle || !map)
    return MS_FAILURE;
  /* ==================================================================== */
  /*      This a definition taken from the specification (11.3.2) :       */
  /*      Graphics can either be referenced from an external URL in a common
   * format (such as*/
  /*      GIF or SVG) or may be derived from a Mark. Multiple external URLs and
   * marks may be*/
  /*      referenced with the semantic that they all provide the equivalent
   * graphic in different*/
  /*      formats.                                                        */
  /*                                                                      */
  /*      For this reason, we only need to support one Mark and one       */
  /*      ExtrnalGraphic ????                                             */
  /* ==================================================================== */
  psGraphic = CPLGetXMLNode(psRoot, "Graphic");
  if (psGraphic) {
    /* extract symbol size */
    psSize = CPLGetXMLNode(psGraphic, "Size");
    if (psSize && psSize->psChild) {
      msSLDParseOgcExpression(psSize->psChild, psStyle, MS_STYLE_BINDING_SIZE,
                              MS_OBJ_STYLE);
    } else {
      /*do not set a default for external symbols #2305*/
      psExternalGraphic = CPLGetXMLNode(psGraphic, "ExternalGraphic");
      if (!psExternalGraphic)
        psStyle->size = 6; /* default value */
    }

    /*SLD 1.1.0 extract opacity, rotation, displacement*/
    psOpacity = CPLGetXMLNode(psGraphic, "Opacity");
    if (psOpacity && psOpacity->psChild) {
      msSLDParseOgcExpression(psOpacity->psChild, psStyle,
                              MS_STYLE_BINDING_OPACITY, MS_OBJ_STYLE);
    }

    psRotation = CPLGetXMLNode(psGraphic, "Rotation");
    if (psRotation && psRotation->psChild) {
      msSLDParseOgcExpression(psRotation->psChild, psStyle,
                              MS_STYLE_BINDING_ANGLE, MS_OBJ_STYLE);
    }
    psDisplacement = CPLGetXMLNode(psGraphic, "Displacement");
    if (psDisplacement && psDisplacement->psChild) {
      psDisplacementX = CPLGetXMLNode(psDisplacement, "DisplacementX");
      if (psDisplacementX && psDisplacementX->psChild) {
        msSLDParseOgcExpression(psDisplacementX->psChild, psStyle,
                                MS_STYLE_BINDING_OFFSET_X, MS_OBJ_STYLE);
      }
      psDisplacementY = CPLGetXMLNode(psDisplacement, "DisplacementY");
      if (psDisplacementY && psDisplacementY->psChild) {
        msSLDParseOgcExpression(psDisplacementY->psChild, psStyle,
                                MS_STYLE_BINDING_OFFSET_Y, MS_OBJ_STYLE);
      }
    }
    /* extract symbol */
    psMark = CPLGetXMLNode(psGraphic, "Mark");
    if (psMark) {
      pszSymbolName = NULL;
      psWellKnownName = CPLGetXMLNode(psMark, "WellKnownName");
      if (psWellKnownName && psWellKnownName->psChild &&
          psWellKnownName->psChild->pszValue)
        pszSymbolName = msStrdup(psWellKnownName->psChild->pszValue);

      /* default symbol is square */

      if (!pszSymbolName || !*pszSymbolName ||
          (strcasecmp(pszSymbolName, "square") != 0 &&
           strcasecmp(pszSymbolName, "circle") != 0 &&
           strcasecmp(pszSymbolName, "triangle") != 0 &&
           strcasecmp(pszSymbolName, "star") != 0 &&
           strcasecmp(pszSymbolName, "cross") != 0 &&
           strcasecmp(pszSymbolName, "x") != 0)) {
        if (!pszSymbolName || !*pszSymbolName ||
            msGetSymbolIndex(&map->symbolset, pszSymbolName, MS_FALSE) < 0) {
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
          psCssParam = CPLGetXMLNode(psFill, "CssParameter");
          /*sld 1.1 used SvgParameter*/
          if (psCssParam == NULL)
            psCssParam = CPLGetXMLNode(psFill, "SvgParameter");

          while (psCssParam && psCssParam->pszValue &&
                 (strcasecmp(psCssParam->pszValue, "CssParameter") == 0 ||
                  strcasecmp(psCssParam->pszValue, "SvgParameter") == 0)) {
            psName = (char *)CPLGetXMLValue(psCssParam, "name", NULL);
            if (psName && strcasecmp(psName, "fill") == 0) {
              if (psCssParam->psChild && psCssParam->psChild->psNext) {
                msSLDParseOgcExpression(psCssParam->psChild->psNext, psStyle,
                                        MS_STYLE_BINDING_COLOR, MS_OBJ_STYLE);
              }
            } else if (psName && strcasecmp(psName, "fill-opacity") == 0) {
              if (psCssParam->psChild && psCssParam->psChild->psNext) {
                psValue = psCssParam->psChild->psNext->pszValue;
                if (psValue) {
                  psStyle->color.alpha = (int)(atof(psValue) * 255);
                }
              }
            }

            psCssParam = psCssParam->psNext;
          }
        }
        if (psStroke) {
          psCssParam = CPLGetXMLNode(psStroke, "CssParameter");
          /*sld 1.1 used SvgParameter*/
          if (psCssParam == NULL)
            psCssParam = CPLGetXMLNode(psStroke, "SvgParameter");

          while (psCssParam && psCssParam->pszValue &&
                 (strcasecmp(psCssParam->pszValue, "CssParameter") == 0 ||
                  strcasecmp(psCssParam->pszValue, "SvgParameter") == 0)) {
            psName = (char *)CPLGetXMLValue(psCssParam, "name", NULL);
            if (psName && strcasecmp(psName, "stroke") == 0) {
              if (psCssParam->psChild && psCssParam->psChild->psNext) {
                if (bFilled) {
                  msSLDParseOgcExpression(psCssParam->psChild->psNext, psStyle,
                                          MS_STYLE_BINDING_OUTLINECOLOR,
                                          MS_OBJ_STYLE);
                } else {
                  msSLDParseOgcExpression(psCssParam->psChild->psNext, psStyle,
                                          MS_STYLE_BINDING_COLOR, MS_OBJ_STYLE);
                }
              }
            } else if (psName && strcasecmp(psName, "stroke-opacity") == 0) {
              if (psCssParam->psChild && psCssParam->psChild->psNext) {
                psValue = psCssParam->psChild->psNext->pszValue;
                if (psValue) {
                  if (bFilled) {
                    psStyle->outlinecolor.alpha = (int)(atof(psValue) * 255);
                  } else {
                    psStyle->color.alpha = (int)(atof(psValue) * 255);
                  }
                }
              }
            } else if (psName && strcasecmp(psName, "stroke-width") == 0) {
              if (psCssParam->psChild && psCssParam->psChild->psNext) {
                msSLDParseOgcExpression(psCssParam->psChild->psNext, psStyle,
                                        MS_STYLE_BINDING_WIDTH, MS_OBJ_STYLE);
              }
            }

            psCssParam = psCssParam->psNext;
          }
        }
      }
      /* set the default color if color is not not already set */
      if ((psStyle->color.red < 0 || psStyle->color.green == -1 ||
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
      if (psStyle->symbol > 0 && psStyle->symbol < map->symbolset.numsymbols)
        psStyle->symbolname =
            msStrdup(map->symbolset.symbol[psStyle->symbol]->name);

    } else {
      psExternalGraphic = CPLGetXMLNode(psGraphic, "ExternalGraphic");
      if (psExternalGraphic)
        msSLDParseExternalGraphic(psExternalGraphic, psStyle, map);
    }
    msFree(pszSymbolName);
  }
  psGap = CPLGetXMLNode(psRoot, "Gap");
  if (psGap && psGap->psChild && psGap->psChild->pszValue) {
    psStyle->gap = atof(psGap->psChild->pszValue);
  }
  psInitialGap = CPLGetXMLNode(psRoot, "InitialGap");
  if (psInitialGap && psInitialGap->psChild &&
      psInitialGap->psChild->pszValue) {
    psStyle->initialgap = atof(psInitialGap->psChild->pszValue);
  }

  return MS_SUCCESS;
}

/************************************************************************/
/*                            msSLDGetMarkSymbol                        */
/*                                                                      */
/*      Get a Mark symbol using the name. Mark symbols can be           */
/*      square, circle, triangle, star, cross, x.                       */
/*      If the symbol does not exist add it to the symbol list.        */
/************************************************************************/
int msSLDGetMarkSymbol(mapObj *map, char *pszSymbolName, int bFilled) {
  int nSymbolId = 0;
  symbolObj *psSymbol = NULL;

  if (!map || !pszSymbolName)
    return 0;

  if (strcasecmp(pszSymbolName, "square") == 0) {
    if (bFilled)
      nSymbolId = msGetSymbolIndex(&map->symbolset,
                                   SLD_MARK_SYMBOL_SQUARE_FILLED, MS_FALSE);
    else
      nSymbolId =
          msGetSymbolIndex(&map->symbolset, SLD_MARK_SYMBOL_SQUARE, MS_FALSE);
  } else if (strcasecmp(pszSymbolName, "circle") == 0) {

    if (bFilled)
      nSymbolId = msGetSymbolIndex(&map->symbolset,
                                   SLD_MARK_SYMBOL_CIRCLE_FILLED, MS_FALSE);
    else
      nSymbolId =
          msGetSymbolIndex(&map->symbolset, SLD_MARK_SYMBOL_CIRCLE, MS_FALSE);
  } else if (strcasecmp(pszSymbolName, "triangle") == 0) {

    if (bFilled)
      nSymbolId = msGetSymbolIndex(&map->symbolset,
                                   SLD_MARK_SYMBOL_TRIANGLE_FILLED, MS_FALSE);
    else
      nSymbolId =
          msGetSymbolIndex(&map->symbolset, SLD_MARK_SYMBOL_TRIANGLE, MS_FALSE);
  } else if (strcasecmp(pszSymbolName, "star") == 0) {

    if (bFilled)
      nSymbolId = msGetSymbolIndex(&map->symbolset, SLD_MARK_SYMBOL_STAR_FILLED,
                                   MS_FALSE);
    else
      nSymbolId =
          msGetSymbolIndex(&map->symbolset, SLD_MARK_SYMBOL_STAR, MS_FALSE);
  } else if (strcasecmp(pszSymbolName, "cross") == 0) {

    if (bFilled)
      nSymbolId = msGetSymbolIndex(&map->symbolset,
                                   SLD_MARK_SYMBOL_CROSS_FILLED, MS_FALSE);
    else
      nSymbolId =
          msGetSymbolIndex(&map->symbolset, SLD_MARK_SYMBOL_CROSS, MS_FALSE);
  } else if (strcasecmp(pszSymbolName, "x") == 0) {

    if (bFilled)
      nSymbolId =
          msGetSymbolIndex(&map->symbolset, SLD_MARK_SYMBOL_X_FILLED, MS_FALSE);
    else
      nSymbolId =
          msGetSymbolIndex(&map->symbolset, SLD_MARK_SYMBOL_X, MS_FALSE);
  } else {
    nSymbolId = msGetSymbolIndex(&map->symbolset, pszSymbolName, MS_FALSE);
  }

  if (nSymbolId <= 0) {
    if ((psSymbol = msGrowSymbolSet(&(map->symbolset))) == NULL)
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

/************************************************************************/
/*                          msSLDGetGraphicSymbol                       */
/*                                                                      */
/*      Create a symbol entry for an inmap pixmap symbol. Returns       */
/*      the symbol id.                                                  */
/************************************************************************/
int msSLDGetGraphicSymbol(mapObj *map, char *pszFileName, char *extGraphicName,
                          int nGap_ignored) {
  (void)nGap_ignored;
  int nSymbolId = 0;
  symbolObj *psSymbol = NULL;

  if (map && pszFileName) {
    if ((psSymbol = msGrowSymbolSet(&(map->symbolset))) == NULL)
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
                              int bNewClass, const char *pszUserStyleName) {
  int nClassId = 0;
  int iStyle = 0;

  if (!psRoot || !psLayer)
    return MS_FAILURE;

  nClassId = getClassId(psLayer, bNewClass, pszUserStyleName);
  if (nClassId < 0)
    return MS_FAILURE;

  // Get uom if any, defaults to MS_PIXELS
  enum MS_UNITS sizeunits = MS_PIXELS;
  if (msSLDParseUomAttribute(psRoot, &sizeunits) != MS_SUCCESS) {
    msSetError(MS_WMSERR, "Invalid uom attribute value.",
               "msSLDParsePolygonSymbolizer()");
    return MS_FAILURE;
  }

  iStyle = psLayer->_class[nClassId]->numstyles;
  msMaybeAllocateClassStyle(psLayer->_class[nClassId], iStyle);
  psLayer->_class[nClassId]->styles[iStyle]->sizeunits = sizeunits;

  msSLDParseGraphicFillOrStroke(
      psRoot, NULL, psLayer->_class[nClassId]->styles[iStyle], psLayer->map);

  return MS_SUCCESS;
}

/************************************************************************/
/*                        msSLDParseExternalGraphic                     */
/*                                                                      */
/*      Parse external graphic node : download the symbol referenced    */
/*      by the URL and create a PIXMAP inmap symbol. Only GIF and       */
/*      PNG are supported.                                              */
/************************************************************************/
int msSLDParseExternalGraphic(CPLXMLNode *psExternalGraphic, styleObj *psStyle,
                              mapObj *map) {
  char *pszFormat = NULL;
  CPLXMLNode *psURL = NULL, *psFormat = NULL, *psTmp = NULL;
  char *pszURL = NULL;

  if (!psExternalGraphic || !psStyle || !map)
    return MS_FAILURE;

  psFormat = CPLGetXMLNode(psExternalGraphic, "Format");
  if (psFormat && psFormat->psChild && psFormat->psChild->pszValue)
    pszFormat = psFormat->psChild->pszValue;

  /* supports GIF and PNG and SVG */
  if (pszFormat && (strcasecmp(pszFormat, "GIF") == 0 ||
                    strcasecmp(pszFormat, "image/gif") == 0 ||
                    strcasecmp(pszFormat, "PNG") == 0 ||
                    strcasecmp(pszFormat, "image/png") == 0 ||
                    strcasecmp(pszFormat, "image/svg+xml") == 0)) {

    /* <OnlineResource xmlns:xlink="http://www.w3.org/1999/xlink"
     * xlink:type="simple" xlink:href="http://www.vendor.com/geosym/2267.svg"/>
     */
    psURL = CPLGetXMLNode(psExternalGraphic, "OnlineResource");
    if (psURL && psURL->psChild) {
      psTmp = psURL->psChild;
      while (psTmp != NULL && psTmp->pszValue &&
             strcasecmp(psTmp->pszValue, "xlink:href") != 0) {
        psTmp = psTmp->psNext;
      }
      if (psTmp && psTmp->psChild) {
        pszURL = (char *)psTmp->psChild->pszValue;

        char *symbolurl = NULL;
        // Handle relative URL for ExternalGraphic
        if (map->sldurl && !strstr(pszURL, "://")) {
          char *baseurl = NULL;
          char *relpath = NULL;
          symbolurl = static_cast<char *>(malloc(sizeof(char) * MS_MAXPATHLEN));
          if (pszURL[0] != '/') {
            // Symbol file is relative to SLD file
            // e.g. SLD   : http://example.com/path/to/sld.xml
            //  and symbol: assets/symbol.svg
            //     lead to: http://example.com/path/to/assets/symbol.svg
            baseurl = msGetPath(map->sldurl);
            relpath = pszURL;
          } else {
            // Symbol file is relative to the root of SLD server
            // e.g. SLD   : http://example.com/path/to/sld.xml
            //  and symbol: /path/to/assets/symbol.svg
            //     lead to: http://example.com/path/to/assets/symbol.svg
            baseurl = msStrdup(map->sldurl);
            relpath = pszURL + 1;
            char *sep = strstr(baseurl, "://");
            if (sep)
              sep += 3;
            else
              sep = baseurl;
            sep = strchr(sep, '/');
            if (!sep)
              sep = baseurl + strlen(baseurl);
            sep[1] = '\0';
          }
          msBuildPath(symbolurl, baseurl, relpath);
          msFree(baseurl);
        } else {
          // Absolute URL
          // e.g. symbol: http://example.com/path/to/assets/symbol.svg
          symbolurl = msStrdup(pszURL);
        }

        /* validate the ExternalGraphic parameter */
        if (msValidateParameter(symbolurl,
                                msLookupHashTable(&(map->web.validation),
                                                  "sld_external_graphic"),
                                NULL, NULL, NULL) != MS_SUCCESS) {
          msSetError(MS_WEBERR,
                     "SLD ExternalGraphic OnlineResource value fails to "
                     "validate against sld_external_graphic VALIDATION",
                     "mapserv()");
          msFree(symbolurl);
          return MS_FAILURE;
        }

        /*external symbols using http will be automaticallly downloaded. The
          file should be saved in a temporary directory (msAddImageSymbol)
          #2305*/
        psStyle->symbol = msGetSymbolIndex(&map->symbolset, symbolurl, MS_TRUE);
        msFree(symbolurl);

        if (psStyle->symbol > 0 && psStyle->symbol < map->symbolset.numsymbols)
          psStyle->symbolname =
              msStrdup(map->symbolset.symbol[psStyle->symbol]->name);

        /* set the color parameter if not set. Does not make sense */
        /* for pixmap but mapserver needs it. */
        if (psStyle->color.red == -1 || psStyle->color.green ||
            psStyle->color.blue) {
          psStyle->color.red = 0;
          psStyle->color.green = 0;
          psStyle->color.blue = 0;
        }
      }
    }
  }

  return MS_SUCCESS;
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
                             int bOtherSymboliser,
                             const char *pszUserStyleName) {
  int nStyleId = 0, nClassId = 0;

  if (!psRoot || !psLayer)
    return MS_FAILURE;

  if (!bOtherSymboliser) {
    if (msGrowLayerClasses(psLayer) == NULL)
      return MS_FAILURE;
    initClass(psLayer->_class[psLayer->numclasses]);
    nClassId = psLayer->numclasses;
    if (pszUserStyleName)
      psLayer->_class[nClassId]->group = msStrdup(pszUserStyleName);
    psLayer->numclasses++;
    msMaybeAllocateClassStyle(psLayer->_class[nClassId], 0);
    nStyleId = 0;
  } else {
    nClassId = psLayer->numclasses - 1;
    if (nClassId >= 0) /* should always be true */
      nStyleId = psLayer->_class[nClassId]->numstyles - 1;
  }

  if (nStyleId >= 0 && nClassId >= 0) /* should always be true */
    msSLDParseTextParams(psRoot, psLayer, psLayer->_class[nClassId]);

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
/*      <xsd:element name="RasterSymbolizer" type="se:RasterSymbolizerType"
 * substitutionGroup="se:Symbolizer"/>*/
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
/*      <xsd:element name="Categorize" type="se:CategorizeType"
 * substitutionGroup="se:Function"/>*/
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
/*      <xsd:attribute name="threshholdsBelongTo"
 * type="se:ThreshholdsBelongToType" use="optional"/>*/
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
int msSLDParseRasterSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer,
                               const char *pszUserStyleName) {
  CPLXMLNode *psColorMap = NULL, *psColorEntry = NULL, *psOpacity = NULL;
  char *pszColor = NULL, *pszQuantity = NULL;
  char *pszPreviousColor = NULL, *pszPreviousQuality = NULL;
  colorObj sColor;
  char szExpression[100];
  double dfOpacity = 1.0;
  char *pszLabel = NULL, *pszPreviousLabel = NULL;
  char *pch = NULL, *pchPrevious = NULL;

  CPLXMLNode *psNode = NULL, *psCategorize = NULL;
  char *pszTmp = NULL;
  int nValues = 0, nThresholds = 0;
  int i, nMaxValues = 100, nMaxThreshold = 100;

  if (!psRoot || !psLayer)
    return MS_FAILURE;

  psOpacity = CPLGetXMLNode(psRoot, "Opacity");
  if (psOpacity) {
    if (psOpacity->psChild && psOpacity->psChild->pszValue)
      dfOpacity = atof(psOpacity->psChild->pszValue);

    /* values in sld goes from 0.0 (for transparent) to 1.0 (for full opacity);
     */
    if (dfOpacity >= 0.0 && dfOpacity <= 1.0)
      msSetLayerOpacity(psLayer, (int)(dfOpacity * 100));
    else {
      msSetError(MS_WMSERR,
                 "Invalid opacity value. Values should be between 0.0 and 1.0",
                 "msSLDParseRasterSymbolizer()");
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
            if (strlen(pszPreviousColor) == 7 && pszPreviousColor[0] == '#' &&
                strlen(pszColor) == 7 && pszColor[0] == '#') {
              sColor.red = msHexToInt(pszPreviousColor + 1);
              sColor.green = msHexToInt(pszPreviousColor + 3);
              sColor.blue = msHexToInt(pszPreviousColor + 5);

              /* pszQuantity and pszPreviousQuality may be integer or float */
              pchPrevious = strchr(pszPreviousQuality, '.');
              pch = strchr(pszQuantity, '.');
              if (pchPrevious == NULL && pch == NULL) {
                snprintf(szExpression, sizeof(szExpression),
                         "([pixel] >= %d AND [pixel] < %d)",
                         atoi(pszPreviousQuality), atoi(pszQuantity));
              } else if (pchPrevious != NULL && pch == NULL) {
                snprintf(szExpression, sizeof(szExpression),
                         "([pixel] >= %f AND [pixel] < %d)",
                         atof(pszPreviousQuality), atoi(pszQuantity));
              } else if (pchPrevious == NULL && pch != NULL) {
                snprintf(szExpression, sizeof(szExpression),
                         "([pixel] >= %d AND [pixel] < %f)",
                         atoi(pszPreviousQuality), atof(pszQuantity));
              } else {
                snprintf(szExpression, sizeof(szExpression),
                         "([pixel] >= %f AND [pixel] < %f)",
                         atof(pszPreviousQuality), atof(pszQuantity));
              }

              if (msGrowLayerClasses(psLayer) == NULL)
                return MS_FAILURE;
              else {
                initClass(psLayer->_class[psLayer->numclasses]);
                psLayer->numclasses++;
                const int nClassId = psLayer->numclasses - 1;
                if (pszUserStyleName)
                  psLayer->_class[nClassId]->group = msStrdup(pszUserStyleName);

                /*set the class name using the label. If label not defined
                  set it with the quantity*/
                if (pszPreviousLabel)
                  psLayer->_class[nClassId]->name = msStrdup(pszPreviousLabel);
                else
                  psLayer->_class[nClassId]->name =
                      msStrdup(pszPreviousQuality);

                msMaybeAllocateClassStyle(psLayer->_class[nClassId], 0);

                psLayer->_class[nClassId]->styles[0]->color.red = sColor.red;
                psLayer->_class[nClassId]->styles[0]->color.green =
                    sColor.green;
                psLayer->_class[nClassId]->styles[0]->color.blue = sColor.blue;

                if (!psLayer->classitem ||
                    strcasecmp(psLayer->classitem, "[pixel]") != 0) {
                  free(psLayer->classitem);
                  psLayer->classitem = msStrdup("[pixel]");
                }

                msLoadExpressionString(&psLayer->_class[nClassId]->expression,
                                       szExpression);
              }
            } else {
              msSetError(MS_WMSERR, "Invalid ColorMap Entry.",
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
          sColor.red = msHexToInt(pszColor + 1);
          sColor.green = msHexToInt(pszColor + 3);
          sColor.blue = msHexToInt(pszColor + 5);

          /* pszQuantity may be integer or float */
          pch = strchr(pszQuantity, '.');
          if (pch == NULL) {
            snprintf(szExpression, sizeof(szExpression), "([pixel] = %d)",
                     atoi(pszQuantity));
          } else {
            snprintf(szExpression, sizeof(szExpression), "([pixel] = %f)",
                     atof(pszQuantity));
          }

          if (msGrowLayerClasses(psLayer) == NULL)
            return MS_FAILURE;
          else {
            initClass(psLayer->_class[psLayer->numclasses]);
            psLayer->numclasses++;
            const int nClassId = psLayer->numclasses - 1;
            if (pszUserStyleName)
              psLayer->_class[nClassId]->group = msStrdup(pszUserStyleName);

            msMaybeAllocateClassStyle(psLayer->_class[nClassId], 0);
            if (pszLabel)
              psLayer->_class[nClassId]->name = msStrdup(pszLabel);
            else
              psLayer->_class[nClassId]->name = msStrdup(pszQuantity);
            psLayer->_class[nClassId]->numstyles = 1;
            psLayer->_class[nClassId]->styles[0]->color.red = sColor.red;
            psLayer->_class[nClassId]->styles[0]->color.green = sColor.green;
            psLayer->_class[nClassId]->styles[0]->color.blue = sColor.blue;

            if (!psLayer->classitem ||
                strcasecmp(psLayer->classitem, "[pixel]") != 0) {
              free(psLayer->classitem);
              psLayer->classitem = msStrdup("[pixel]");
            }

            msLoadExpressionString(&psLayer->_class[nClassId]->expression,
                                   szExpression);
          }
        }
      }
    } else if ((psCategorize = CPLGetXMLNode(psColorMap, "Categorize"))) {
      char **papszValues = (char **)msSmallMalloc(sizeof(char *) * nMaxValues);
      char **papszThresholds =
          (char **)msSmallMalloc(sizeof(char *) * nMaxThreshold);
      psNode = CPLGetXMLNode(psCategorize, "Value");
      while (psNode && psNode->pszValue && psNode->psChild &&
             psNode->psChild->pszValue)

      {
        if (strcasecmp(psNode->pszValue, "Value") == 0) {
          papszValues[nValues] = psNode->psChild->pszValue;
          nValues++;
          if (nValues == nMaxValues) {
            nMaxValues += 100;
            papszValues = (char **)msSmallRealloc(papszValues,
                                                  sizeof(char *) * nMaxValues);
          }
        } else if (strcasecmp(psNode->pszValue, "Threshold") == 0) {
          papszThresholds[nThresholds] = psNode->psChild->pszValue;
          nThresholds++;
          if (nValues == nMaxThreshold) {
            nMaxThreshold += 100;
            papszThresholds = (char **)msSmallRealloc(
                papszThresholds, sizeof(char *) * nMaxThreshold);
          }
        }
        psNode = psNode->psNext;
      }

      if (nThresholds > 0 && nValues == nThresholds + 1) {
        /*free existing classes*/
        for (i = 0; i < psLayer->numclasses; i++) {
          if (psLayer->_class[i] != NULL) {
            psLayer->_class[i]->layer = NULL;
            if (freeClass(psLayer->_class[i]) == MS_SUCCESS) {
              msFree(psLayer->_class[i]);
              psLayer->_class[i] = NULL;
            }
          }
        }
        psLayer->numclasses = 0;
        for (i = 0; i < nValues; i++) {
          pszTmp = (papszValues[i]);
          if (pszTmp && strlen(pszTmp) == 7 && pszTmp[0] == '#') {
            sColor.red = msHexToInt(pszTmp + 1);
            sColor.green = msHexToInt(pszTmp + 3);
            sColor.blue = msHexToInt(pszTmp + 5);
            if (i == 0) {
              if (strchr(papszThresholds[i], '.'))
                snprintf(szExpression, sizeof(szExpression), "([pixel] < %f)",
                         atof(papszThresholds[i]));
              else
                snprintf(szExpression, sizeof(szExpression), "([pixel] < %d)",
                         atoi(papszThresholds[i]));

            } else if (i < nValues - 1) {
              if (strchr(papszThresholds[i], '.'))
                snprintf(szExpression, sizeof(szExpression),
                         "([pixel] >= %f AND [pixel] < %f)",
                         atof(papszThresholds[i - 1]),
                         atof(papszThresholds[i]));
              else
                snprintf(szExpression, sizeof(szExpression),
                         "([pixel] >= %d AND [pixel] < %d)",
                         atoi(papszThresholds[i - 1]),
                         atoi(papszThresholds[i]));
            } else {
              if (strchr(papszThresholds[i - 1], '.'))
                snprintf(szExpression, sizeof(szExpression), "([pixel] >= %f)",
                         atof(papszThresholds[i - 1]));
              else
                snprintf(szExpression, sizeof(szExpression), "([pixel] >= %d)",
                         atoi(papszThresholds[i - 1]));
            }
            if (msGrowLayerClasses(psLayer)) {
              initClass(psLayer->_class[psLayer->numclasses]);
              psLayer->numclasses++;
              const int nClassId = psLayer->numclasses - 1;
              if (pszUserStyleName)
                psLayer->_class[nClassId]->group = msStrdup(pszUserStyleName);
              msMaybeAllocateClassStyle(psLayer->_class[nClassId], 0);
              psLayer->_class[nClassId]->numstyles = 1;
              psLayer->_class[nClassId]->styles[0]->color.red = sColor.red;
              psLayer->_class[nClassId]->styles[0]->color.green = sColor.green;
              psLayer->_class[nClassId]->styles[0]->color.blue = sColor.blue;
              if (psLayer->classitem &&
                  strcasecmp(psLayer->classitem, "[pixel]") != 0)
                free(psLayer->classitem);
              psLayer->classitem = msStrdup("[pixel]");
              msLoadExpressionString(&psLayer->_class[nClassId]->expression,
                                     szExpression);
            }
          }
        }
      }
      msFree(papszValues);
      msFree(papszThresholds);

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
/*      Parse text parameters like font, placement and color.           */
/************************************************************************/
int msSLDParseTextParams(CPLXMLNode *psRoot, layerObj *psLayer,
                         classObj *psClass) {
  char szFontName[100];

  CPLXMLNode *psLabel = NULL, *psFont = NULL;
  CPLXMLNode *psCssParam = NULL;
  char *pszName = NULL, *pszFontFamily = NULL, *pszFontStyle = NULL;
  char *pszFontWeight = NULL;
  CPLXMLNode *psLabelPlacement = NULL, *psPointPlacement = NULL,
             *psLinePlacement = NULL;
  CPLXMLNode *psFill = NULL, *psHalo = NULL, *psHaloRadius = NULL,
             *psHaloFill = NULL;
  labelObj *psLabelObj = NULL;
  szFontName[0] = '\0';

  if (!psRoot || !psClass || !psLayer)
    return MS_FAILURE;

  if (psClass->numlabels == 0) {
    if (msGrowClassLabels(psClass) == NULL)
      return (MS_FAILURE);
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
   -
  <TextSymbolizer><Label><ogc:PropertyName>MY_COLUMN</ogc:PropertyName></Label>
  Bug 1857 */
  psLabel = CPLGetXMLNode(psRoot, "Label");
  if (psLabel) {
    const char *sep = "";
    msStringBuffer *classtext = msStringBufferAlloc();
    msStringBufferAppend(classtext, "(");
    for (CPLXMLNode *psTmpNode = psLabel->psChild; psTmpNode;
         psTmpNode = psTmpNode->psNext) {
      if (psTmpNode->eType == CXT_Text && psTmpNode->pszValue) {
        msStringBufferAppend(classtext, sep);
        msStringBufferAppend(classtext, "\"");
        msStringBufferAppend(classtext, psTmpNode->pszValue);
        msStringBufferAppend(classtext, "\"");
        sep = "+";
      } else if (psTmpNode->eType == CXT_Element &&
                 strcasecmp(psTmpNode->pszValue, "Literal") == 0 &&
                 psTmpNode->psChild) {
        msStringBufferAppend(classtext, sep);
        msStringBufferAppend(classtext, "\"");
        msStringBufferAppend(classtext, psTmpNode->psChild->pszValue);
        msStringBufferAppend(classtext, "\"");
        sep = "+";
      } else if (psTmpNode->eType == CXT_Element &&
                 strcasecmp(psTmpNode->pszValue, "PropertyName") == 0 &&
                 psTmpNode->psChild) {
        msStringBufferAppend(classtext, sep);
        msStringBufferAppend(classtext, "\"[");
        msStringBufferAppend(classtext, psTmpNode->psChild->pszValue);
        msStringBufferAppend(classtext, "]\"");
        sep = "+";
      } else if (psTmpNode->eType == CXT_Element &&
                 strcasecmp(psTmpNode->pszValue, "Function") == 0 &&
                 psTmpNode->psChild) {
        msStringBufferAppend(classtext, sep);
        msStringBufferAppend(classtext, "tostring(");

        labelObj tempExpressionCollector;
        initLabel(&tempExpressionCollector);
        msSLDParseOgcExpression(psTmpNode, &tempExpressionCollector,
                                MS_LABEL_BINDING_SIZE, MS_OBJ_LABEL);
        msStringBufferAppend(
            classtext,
            tempExpressionCollector.exprBindings[MS_LABEL_BINDING_SIZE].string);
        freeLabel(&tempExpressionCollector);

        msStringBufferAppend(classtext, ",\"%g\")");
        sep = "+";
      }
    }
    msStringBufferAppend(classtext, ")");
    const char *expressionstring = msStringBufferGetString(classtext);
    if (strlen(expressionstring) > 2) {
      msLoadExpressionString(&psClass->text, (char *)expressionstring);
    }
    msStringBufferFree(classtext);

    {
      /* font */
      psFont = CPLGetXMLNode(psRoot, "Font");
      if (psFont) {
        psCssParam = CPLGetXMLNode(psFont, "CssParameter");
        /*sld 1.1 used SvgParameter*/
        if (psCssParam == NULL)
          psCssParam = CPLGetXMLNode(psFont, "SvgParameter");

        while (psCssParam && psCssParam->pszValue &&
               (strcasecmp(psCssParam->pszValue, "CssParameter") == 0 ||
                strcasecmp(psCssParam->pszValue, "SvgParameter") == 0)) {
          pszName = (char *)CPLGetXMLValue(psCssParam, "name", NULL);
          if (pszName) {
            if (strcasecmp(pszName, "font-family") == 0) {
              if (psCssParam->psChild && psCssParam->psChild->psNext &&
                  psCssParam->psChild->psNext->pszValue)
                pszFontFamily = psCssParam->psChild->psNext->pszValue;
            }
            /* normal, italic, oblique */
            else if (strcasecmp(pszName, "font-style") == 0) {
              if (psCssParam->psChild && psCssParam->psChild->psNext &&
                  psCssParam->psChild->psNext->pszValue)
                pszFontStyle = psCssParam->psChild->psNext->pszValue;
            }
            /* normal or bold */
            else if (strcasecmp(pszName, "font-weight") == 0) {
              if (psCssParam->psChild && psCssParam->psChild->psNext &&
                  psCssParam->psChild->psNext->pszValue)
                pszFontWeight = psCssParam->psChild->psNext->pszValue;
            }
            /* default is 10 pix */
            else if (strcasecmp(pszName, "font-size") == 0) {
              if (psCssParam->psChild && psCssParam->psChild->psNext) {
                msSLDParseOgcExpression(psCssParam->psChild->psNext, psLabelObj,
                                        MS_LABEL_BINDING_SIZE, MS_OBJ_LABEL);
              }
            }
          }
          psCssParam = psCssParam->psNext;
        }
      }

      /* -------------------------------------------------------------------- */
      /*      parse the label placement.                                      */
      /* -------------------------------------------------------------------- */
      psLabelPlacement = CPLGetXMLNode(psRoot, "LabelPlacement");
      if (psLabelPlacement) {
        psPointPlacement = CPLGetXMLNode(psLabelPlacement, "PointPlacement");
        psLinePlacement = CPLGetXMLNode(psLabelPlacement, "LinePlacement");
        if (psPointPlacement)
          ParseTextPointPlacement(psPointPlacement, psClass);
        if (psLinePlacement)
          ParseTextLinePlacement(psLinePlacement, psClass);
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

        if ((msLookupHashTable(&(psLayer->map->fontset.fonts), szFontName) !=
             NULL)) {
          psLabelObj->font = msStrdup(szFontName);
        }
      }

      /* -------------------------------------------------------------------- */
      /*      parse the halo parameter.                                       */
      /* -------------------------------------------------------------------- */
      psHalo = CPLGetXMLNode(psRoot, "Halo");
      if (psHalo) {
        psHaloRadius = CPLGetXMLNode(psHalo, "Radius");
        if (psHaloRadius && psHaloRadius->psChild &&
            psHaloRadius->psChild->pszValue)
          psLabelObj->outlinewidth = atoi(psHaloRadius->psChild->pszValue);

        psHaloFill = CPLGetXMLNode(psHalo, "Fill");
        if (psHaloFill) {
          psCssParam = CPLGetXMLNode(psHaloFill, "CssParameter");
          /*sld 1.1 used SvgParameter*/
          if (psCssParam == NULL)
            psCssParam = CPLGetXMLNode(psHaloFill, "SvgParameter");

          while (psCssParam && psCssParam->pszValue &&
                 (strcasecmp(psCssParam->pszValue, "CssParameter") == 0 ||
                  strcasecmp(psCssParam->pszValue, "SvgParameter") == 0)) {
            pszName = (char *)CPLGetXMLValue(psCssParam, "name", NULL);
            if (pszName) {
              if (strcasecmp(pszName, "fill") == 0) {
                if (psCssParam->psChild && psCssParam->psChild->psNext) {
                  msSLDParseOgcExpression(
                      psCssParam->psChild->psNext, psLabelObj,
                      MS_LABEL_BINDING_OUTLINECOLOR, MS_OBJ_LABEL);
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
        psCssParam = CPLGetXMLNode(psFill, "CssParameter");
        /*sld 1.1 used SvgParameter*/
        if (psCssParam == NULL)
          psCssParam = CPLGetXMLNode(psFill, "SvgParameter");

        while (psCssParam && psCssParam->pszValue &&
               (strcasecmp(psCssParam->pszValue, "CssParameter") == 0 ||
                strcasecmp(psCssParam->pszValue, "SvgParameter") == 0)) {
          pszName = (char *)CPLGetXMLValue(psCssParam, "name", NULL);
          if (pszName) {
            if (strcasecmp(pszName, "fill") == 0) {
              if (psCssParam->psChild && psCssParam->psChild->psNext) {
                msSLDParseOgcExpression(psCssParam->psChild->psNext, psLabelObj,
                                        MS_LABEL_BINDING_COLOR, MS_OBJ_LABEL);
              }
            }
          }
          psCssParam = psCssParam->psNext;
        }
      }
    }
  }

  return MS_SUCCESS;
}

/************************************************************************/
/*                         ParseTextPointPlacement                      */
/*                                                                      */
/*      point placement node for the text symbolizer.                  */
/************************************************************************/
int ParseTextPointPlacement(CPLXMLNode *psRoot, classObj *psClass) {
  CPLXMLNode *psAnchor, *psAnchorX, *psAnchorY;
  CPLXMLNode *psDisplacement, *psDisplacementX, *psDisplacementY;
  CPLXMLNode *psRotation = NULL;
  labelObj *psLabelObj = NULL;

  if (!psRoot || !psClass)
    return MS_FAILURE;
  if (psClass->numlabels == 0) {
    if (msGrowClassLabels(psClass) == NULL)
      return (MS_FAILURE);
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
    if (psAnchorX && psAnchorX->psChild && psAnchorX->psChild->pszValue &&
        psAnchorY && psAnchorY->psChild && psAnchorY->psChild->pszValue) {
      const double dfAnchorX = atof(psAnchorX->psChild->pszValue);
      const double dfAnchorY = atof(psAnchorY->psChild->pszValue);

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
    if (psDisplacementX && psDisplacementX->psChild &&
        psDisplacementX->psChild->pszValue && psDisplacementY &&
        psDisplacementY->psChild && psDisplacementY->psChild->pszValue) {
      psLabelObj->offsetx = atoi(psDisplacementX->psChild->pszValue);
      psLabelObj->offsety = atoi(psDisplacementY->psChild->pszValue);
    }
  }

  /* -------------------------------------------------------------------- */
  /*      parse rotation.                                                 */
  /* -------------------------------------------------------------------- */
  psRotation = CPLGetXMLNode(psRoot, "Rotation");
  if (psRotation && psRotation->psChild) {
    msSLDParseOgcExpression(psRotation->psChild, psLabelObj,
                            MS_LABEL_BINDING_ANGLE, MS_OBJ_LABEL);
  }

  return MS_SUCCESS;
}

/************************************************************************/
/*                          ParseTextLinePlacement                      */
/*                                                                      */
/*      Lineplacement node fro the text symbolizer.                     */
/************************************************************************/
int ParseTextLinePlacement(CPLXMLNode *psRoot, classObj *psClass) {
  CPLXMLNode *psOffset = NULL, *psAligned = NULL;
  labelObj *psLabelObj = NULL;

  if (!psRoot || !psClass)
    return MS_FAILURE;

  if (psClass->numlabels == 0) {
    if (msGrowClassLabels(psClass) == NULL)
      return (MS_FAILURE);
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
    psLabelObj->offsety = MS_LABEL_PERPENDICULAR_OFFSET;

    /*if there is a PerpendicularOffset, we will assume that the
      best setting for mapserver would be to use angle=0 and the
      the offset #2806*/
    /* since sld 1.1.0 introduces the IsAligned parameter, only
       set the angles if the parameter is not set*/
    if (!psAligned) {
      psLabelObj->anglemode = MS_NONE;
      psLabelObj->offsety = psLabelObj->offsetx;
    }
  }

  return MS_SUCCESS;
}

/************************************************************************/
/*           void msSLDSetColorObject(char *psHexColor, colorObj        */
/*      *psColor)                                                       */
/*                                                                      */
/*      Utility function to extract rgb values from an hexadecimal     */
/*      color string (format is : #aaff08) and set it in the color      */
/*      object.                                                         */
/************************************************************************/
int msSLDSetColorObject(char *psHexColor, colorObj *psColor) {
  if (psHexColor && psColor && strlen(psHexColor) == 7 &&
      psHexColor[0] == '#') {

    psColor->red = msHexToInt(psHexColor + 1);
    psColor->green = msHexToInt(psHexColor + 3);
    psColor->blue = msHexToInt(psHexColor + 5);
  }

  return MS_SUCCESS;
}

/* -------------------------------------------------------------------- */
/*      client sld support functions                                    */
/* -------------------------------------------------------------------- */

/************************************************************************/
/*                msSLDGenerateSLD(mapObj *map, int iLayer)             */
/*                                                                      */
/*      Return an SLD document for all layers that are on or            */
/*      default. The second argument should be set to -1 to generate    */
/*      on all layers. Or set to the layer index to generate an SLD     */
/*      for a specific layer.                                           */
/*                                                                      */
/*      The caller should free the returned string.                     */
/************************************************************************/
char *msSLDGenerateSLD(mapObj *map, int iLayer, const char *pszVersion) {
#if defined(USE_WMS_SVR) || defined(USE_WFS_SVR) || defined(USE_WCS_SVR) ||    \
    defined(USE_SOS_SVR)

  char szTmp[500];
  int i = 0;
  char *pszTmp = NULL;
  char *pszSLD = NULL;
  char *schemalocation = NULL;
  int sld_version = OWS_VERSION_NOTSET;

  sld_version = msOWSParseVersionString(pszVersion);

  if (sld_version == OWS_VERSION_NOTSET ||
      (sld_version != OWS_1_0_0 && sld_version != OWS_1_1_0))
    sld_version = OWS_1_0_0;

  if (map) {
    schemalocation = msEncodeHTMLEntities(msOWSGetSchemasLocation(map));
    if (sld_version == OWS_1_0_0)
      snprintf(szTmp, sizeof(szTmp),
               "<StyledLayerDescriptor version=\"1.0.0\" "
               "xmlns=\"http://www.opengis.net/sld\" "
               "xmlns:gml=\"http://www.opengis.net/gml\" "
               "xmlns:ogc=\"http://www.opengis.net/ogc\" "
               "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
               "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
               "xsi:schemaLocation=\"http://www.opengis.net/sld "
               "%s/sld/1.0.0/StyledLayerDescriptor.xsd\">\n",
               schemalocation);
    else
      snprintf(szTmp, sizeof(szTmp),
               "<StyledLayerDescriptor version=\"1.1.0\" "
               "xsi:schemaLocation=\"http://www.opengis.net/sld "
               "%s/sld/1.1.0/StyledLayerDescriptor.xsd\" "
               "xmlns=\"http://www.opengis.net/sld\" "
               "xmlns:ogc=\"http://www.opengis.net/ogc\" "
               "xmlns:se=\"http://www.opengis.net/se\" "
               "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
               "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n",
               schemalocation);

    free(schemalocation);

    pszSLD = msStringConcatenate(pszSLD, szTmp);
    if (iLayer < 0 || iLayer > map->numlayers - 1) {
      for (i = 0; i < map->numlayers; i++) {
        pszTmp = msSLDGenerateSLDLayer(GET_LAYER(map, i), sld_version);
        if (pszTmp) {
          pszSLD = msStringConcatenate(pszSLD, pszTmp);
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
  msSetError(MS_MISCERR, "OWS support is not available.",
             "msSLDGenerateSLDLayer()");
  return NULL;

#endif
}

/************************************************************************/
/*                            msSLDGetGraphicSLD                        */
/*                                                                      */
/*      Get an SLD for a style containing a symbol (Mark or external).  */
/************************************************************************/
char *msSLDGetGraphicSLD(styleObj *psStyle, layerObj *psLayer,
                         int bNeedMarkSybol, int nVersion) {
#if defined(USE_WMS_SVR) || defined(USE_WFS_SVR) || defined(USE_WCS_SVR) ||    \
    defined(USE_SOS_SVR)

  msStringBuffer *sldString = msStringBufferAlloc();
  int nSymbol = -1;
  symbolObj *psSymbol = NULL;
  char szTmp[512];
  char szFormat[4];
  int i = 0, nLength = 0;
  int bColorAvailable = 0;
  int bGenerateDefaultSymbol = 0;
  char *pszSymbolName = NULL;
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
      nSymbol = msGetSymbolIndex(&psLayer->map->symbolset, psStyle->symbolname,
                                 MS_FALSE);

    bGenerateDefaultSymbol = 0;

    if (bNeedMarkSybol &&
        (nSymbol <= 0 || nSymbol >= psLayer->map->symbolset.numsymbols))
      bGenerateDefaultSymbol = 1;

    if (nSymbol > 0 && nSymbol < psLayer->map->symbolset.numsymbols) {
      psSymbol = psLayer->map->symbolset.symbol[nSymbol];
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
          else if (strncasecmp(psSymbol->name, "sld_mark_symbol_square", 22) ==
                   0)
            pszSymbolName = msStrdup("square");
          else if (strncasecmp(psSymbol->name, "sld_mark_symbol_triangle",
                               24) == 0)
            pszSymbolName = msStrdup("triangle");
          else if (strncasecmp(psSymbol->name, "sld_mark_symbol_circle", 22) ==
                   0)
            pszSymbolName = msStrdup("circle");
          else if (strncasecmp(psSymbol->name, "sld_mark_symbol_star", 20) == 0)
            pszSymbolName = msStrdup("star");
          else if (strncasecmp(psSymbol->name, "sld_mark_symbol_cross", 21) ==
                   0)
            pszSymbolName = msStrdup("cross");
          else if (strncasecmp(psSymbol->name, "sld_mark_symbol_x", 17) == 0)
            pszSymbolName = msStrdup("X");

          if (pszSymbolName) {
            colorObj sTmpFillColor = {128, 128, 128, 255};
            colorObj sTmpStrokeColor = {0, 0, 0, 255};
            int hasFillColor = 0;
            int hasStrokeColor = 0;

            snprintf(szTmp, sizeof(szTmp), "<%sGraphic>\n", sNameSpace);
            msStringBufferAppend(sldString, szTmp);

            snprintf(szTmp, sizeof(szTmp), "<%sMark>\n", sNameSpace);
            msStringBufferAppend(sldString, szTmp);

            snprintf(szTmp, sizeof(szTmp),
                     "<%sWellKnownName>%s</%sWellKnownName>\n", sNameSpace,
                     pszSymbolName, sNameSpace);
            msStringBufferAppend(sldString, szTmp);

            if (psStyle->color.red != -1 && psStyle->color.green != -1 &&
                psStyle->color.blue != -1) {
              sTmpFillColor.red = psStyle->color.red;
              sTmpFillColor.green = psStyle->color.green;
              sTmpFillColor.blue = psStyle->color.blue;
              sTmpFillColor.alpha = psStyle->color.alpha;
              hasFillColor = 1;
            }
            if (psStyle->outlinecolor.red != -1 &&
                psStyle->outlinecolor.green != -1 &&
                psStyle->outlinecolor.blue != -1) {
              sTmpStrokeColor.red = psStyle->outlinecolor.red;
              sTmpStrokeColor.green = psStyle->outlinecolor.green;
              sTmpStrokeColor.blue = psStyle->outlinecolor.blue;
              sTmpStrokeColor.alpha = psStyle->outlinecolor.alpha;
              hasStrokeColor = 1;
              // Make defaults implicit
              if (sTmpStrokeColor.red == 0 && sTmpStrokeColor.green == 0 &&
                  sTmpStrokeColor.blue == 0 && sTmpStrokeColor.alpha == 255 &&
                  psStyle->width == 1) {
                hasStrokeColor = 0;
              }
            }
            if (!hasFillColor && !hasStrokeColor) {
              sTmpFillColor.red = 128;
              sTmpFillColor.green = 128;
              sTmpFillColor.blue = 128;
              sTmpFillColor.alpha = 255;
              hasFillColor = 1;
            }

            if (hasFillColor) {
              snprintf(szTmp, sizeof(szTmp), "<%sFill>\n", sNameSpace);
              msStringBufferAppend(sldString, szTmp);
              snprintf(szTmp, sizeof(szTmp),
                       "<%s name=\"fill\">#%02x%02x%02x</%s>\n", sCssParam,
                       sTmpFillColor.red, sTmpFillColor.green,
                       sTmpFillColor.blue, sCssParam);
              msStringBufferAppend(sldString, szTmp);
              if (sTmpFillColor.alpha != 255 && sTmpFillColor.alpha != -1) {
                snprintf(szTmp, sizeof(szTmp),
                         "<%s name=\"fill-opacity\">%.2f</%s>\n", sCssParam,
                         (float)sTmpFillColor.alpha / 255.0, sCssParam);
                msStringBufferAppend(sldString, szTmp);
              }
              snprintf(szTmp, sizeof(szTmp), "</%sFill>\n", sNameSpace);
              msStringBufferAppend(sldString, szTmp);
            }
            if (hasStrokeColor) {
              snprintf(szTmp, sizeof(szTmp), "<%sStroke>\n", sNameSpace);
              msStringBufferAppend(sldString, szTmp);
              snprintf(szTmp, sizeof(szTmp),
                       "<%s name=\"stroke\">#%02x%02x%02x</%s>\n", sCssParam,
                       sTmpStrokeColor.red, sTmpStrokeColor.green,
                       sTmpStrokeColor.blue, sCssParam);
              msStringBufferAppend(sldString, szTmp);
              if (psStyle->width > 0) {
                snprintf(szTmp, sizeof(szTmp),
                         "<%s name=\"stroke-width\">%g</%s>\n", sCssParam,
                         psStyle->width, sCssParam);
                msStringBufferAppend(sldString, szTmp);
              }
              if (sTmpStrokeColor.alpha != 255 && sTmpStrokeColor.alpha != -1) {
                snprintf(szTmp, sizeof(szTmp),
                         "<%s name=\"stroke-opacity\">%.2f</%s>\n", sCssParam,
                         (float)sTmpStrokeColor.alpha / 255.0, sCssParam);
                msStringBufferAppend(sldString, szTmp);
              }
              snprintf(szTmp, sizeof(szTmp), "</%sStroke>\n", sNameSpace);
              msStringBufferAppend(sldString, szTmp);
            }

            snprintf(szTmp, sizeof(szTmp), "</%sMark>\n", sNameSpace);
            msStringBufferAppend(sldString, szTmp);

            if (psStyle->size > 0) {
              snprintf(szTmp, sizeof(szTmp), "<%sSize>%g</%sSize>\n",
                       sNameSpace, psStyle->size, sNameSpace);
              msStringBufferAppend(sldString, szTmp);
            }

            if (fmod(psStyle->angle, 360)) {
              snprintf(szTmp, sizeof(szTmp), "<%sRotation>%g</%sRotation>\n",
                       sNameSpace, psStyle->angle, sNameSpace);
              msStringBufferAppend(sldString, szTmp);
            }
            // Style opacity is already reported to alpha channel of color and
            // outlinecolor if (psStyle->opacity < 100)
            // {
            //   snprintf(szTmp, sizeof(szTmp), "<%sOpacity>%g</%sOpacity>\n",
            //       sNameSpace, psStyle->opacity/100.0, sNameSpace);
            //   pszSLD = msStringConcatenate(pszSLD, szTmp);
            // }

            if (psStyle->offsetx != 0 || psStyle->offsety != 0) {
              snprintf(szTmp, sizeof(szTmp), "<%sDisplacement>\n", sNameSpace);
              msStringBufferAppend(sldString, szTmp);
              snprintf(szTmp, sizeof(szTmp),
                       "<%sDisplacementX>%g</%sDisplacementX>\n", sNameSpace,
                       psStyle->offsetx, sNameSpace);
              msStringBufferAppend(sldString, szTmp);
              snprintf(szTmp, sizeof(szTmp),
                       "<%sDisplacementY>%g</%sDisplacementY>\n", sNameSpace,
                       psStyle->offsety, sNameSpace);
              msStringBufferAppend(sldString, szTmp);
              snprintf(szTmp, sizeof(szTmp), "</%sDisplacement>\n", sNameSpace);
              msStringBufferAppend(sldString, szTmp);
            }

            snprintf(szTmp, sizeof(szTmp), "</%sGraphic>\n", sNameSpace);
            msStringBufferAppend(sldString, szTmp);

            if (pszSymbolName)
              free(pszSymbolName);
          }
        } else
          bGenerateDefaultSymbol = 1;
      } else if (psSymbol->type == MS_SYMBOL_PIXMAP ||
                 psSymbol->type == MS_SYMBOL_SVG) {
        if (psSymbol->name) {
          const char *pszURL =
              msLookupHashTable(&(psLayer->metadata), "WMS_SLD_SYMBOL_URL");
          if (!pszURL)
            pszURL = msLookupHashTable(&(psLayer->map->web.metadata),
                                       "WMS_SLD_SYMBOL_URL");

          if (pszURL) {
            snprintf(szTmp, sizeof(szTmp), "<%sGraphic>\n", sNameSpace);
            msStringBufferAppend(sldString, szTmp);

            snprintf(szTmp, sizeof(szTmp), "<%sExternalGraphic>\n", sNameSpace);
            msStringBufferAppend(sldString, szTmp);

            snprintf(szTmp, sizeof(szTmp),
                     "<%sOnlineResource "
                     "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
                     "xlink:type=\"simple\" xlink:href=\"%s%s\"/>\n",
                     sNameSpace, pszURL, psSymbol->imagepath);
            msStringBufferAppend(sldString, szTmp);
            /* TODO : extract format from symbol */

            szFormat[0] = '\0';
            nLength = strlen(psSymbol->imagepath);
            if (nLength > 3) {
              for (i = 0; i <= 2; i++)
                szFormat[2 - i] = psSymbol->imagepath[nLength - 1 - i];
              szFormat[3] = '\0';
            }
            if (strlen(szFormat) > 0 && ((strcasecmp(szFormat, "GIF") == 0) ||
                                         (strcasecmp(szFormat, "PNG") == 0))) {
              if (strcasecmp(szFormat, "GIF") == 0)
                snprintf(szTmp, sizeof(szTmp),
                         "<%sFormat>image/gif</%sFormat>\n", sNameSpace,
                         sNameSpace);
              else
                snprintf(szTmp, sizeof(szTmp),
                         "<%sFormat>image/png</%sFormat>\n", sNameSpace,
                         sNameSpace);
            } else
              snprintf(szTmp, sizeof(szTmp), "<%sFormat>%s</%sFormat>\n",
                       sNameSpace,
                       (psSymbol->type == MS_SYMBOL_SVG) ? "image/svg+xml"
                                                         : "image/gif",
                       sNameSpace);

            msStringBufferAppend(sldString, szTmp);

            snprintf(szTmp, sizeof(szTmp), "</%sExternalGraphic>\n",
                     sNameSpace);
            msStringBufferAppend(sldString, szTmp);

            if (psStyle->size > 0)
              snprintf(szTmp, sizeof(szTmp), "<%sSize>%g</%sSize>\n",
                       sNameSpace, psStyle->size, sNameSpace);
            msStringBufferAppend(sldString, szTmp);

            snprintf(szTmp, sizeof(szTmp), "</%sGraphic>\n", sNameSpace);
            msStringBufferAppend(sldString, szTmp);
          }
        }
      }
    }
    if (bGenerateDefaultSymbol) { /* generate a default square symbol */
      snprintf(szTmp, sizeof(szTmp), "<%sGraphic>\n", sNameSpace);
      msStringBufferAppend(sldString, szTmp);

      snprintf(szTmp, sizeof(szTmp), "<%sMark>\n", sNameSpace);
      msStringBufferAppend(sldString, szTmp);

      snprintf(szTmp, sizeof(szTmp), "<%sWellKnownName>%s</%sWellKnownName>\n",
               sNameSpace, "square", sNameSpace);
      msStringBufferAppend(sldString, szTmp);

      bColorAvailable = 0;
      if (psStyle->color.red != -1 && psStyle->color.green != -1 &&
          psStyle->color.blue != -1) {
        snprintf(szTmp, sizeof(szTmp), "<%sFill>\n", sNameSpace);
        msStringBufferAppend(sldString, szTmp);
        snprintf(szTmp, sizeof(szTmp), "<%s name=\"fill\">#%02x%02x%02x</%s>\n",
                 sCssParam, psStyle->color.red, psStyle->color.green,
                 psStyle->color.blue, sCssParam);
        msStringBufferAppend(sldString, szTmp);
        snprintf(szTmp, sizeof(szTmp), "</%sFill>\n", sNameSpace);
        msStringBufferAppend(sldString, szTmp);
        bColorAvailable = 1;
      }
      if (psStyle->outlinecolor.red != -1 &&
          psStyle->outlinecolor.green != -1 &&
          psStyle->outlinecolor.blue != -1) {
        snprintf(szTmp, sizeof(szTmp), "<%sStroke>\n", sNameSpace);
        msStringBufferAppend(sldString, szTmp);
        snprintf(szTmp, sizeof(szTmp),
                 "<%s name=\"Stroke\">#%02x%02x%02x</%s>\n", sCssParam,
                 psStyle->outlinecolor.red, psStyle->outlinecolor.green,
                 psStyle->outlinecolor.blue, sCssParam);
        msStringBufferAppend(sldString, szTmp);
        snprintf(szTmp, sizeof(szTmp), "</%sStroke>\n", sNameSpace);
        msStringBufferAppend(sldString, szTmp);
        bColorAvailable = 1;
      }
      if (!bColorAvailable) {
        /* default color */
        snprintf(szTmp, sizeof(szTmp), "<%sFill>\n", sNameSpace);
        msStringBufferAppend(sldString, szTmp);
        snprintf(szTmp, sizeof(szTmp), "<%s name=\"fill\">%s</%s>\n", sCssParam,
                 "#808080", sCssParam);
        msStringBufferAppend(sldString, szTmp);
        snprintf(szTmp, sizeof(szTmp), "</%sFill>\n", sNameSpace);
        msStringBufferAppend(sldString, szTmp);
      }

      snprintf(szTmp, sizeof(szTmp), "</%sMark>\n", sNameSpace);
      msStringBufferAppend(sldString, szTmp);

      if (psStyle->size > 0)
        snprintf(szTmp, sizeof(szTmp), "<%sSize>%g</%sSize>\n", sNameSpace,
                 psStyle->size, sNameSpace);
      else
        snprintf(szTmp, sizeof(szTmp), "<%sSize>%d</%sSize>\n", sNameSpace, 1,
                 sNameSpace);
      msStringBufferAppend(sldString, szTmp);

      snprintf(szTmp, sizeof(szTmp), "</%sGraphic>\n", sNameSpace);
      msStringBufferAppend(sldString, szTmp);
    }
  }

  return msStringBufferReleaseStringAndFree(sldString);

#else
  return NULL;

#endif
}

/************************************************************************/
/*                           msSLDGenerateLineSLD                       */
/*                                                                      */
/*      Generate SLD for a Line layer.                                  */
/************************************************************************/
char *msSLDGenerateLineSLD(styleObj *psStyle, layerObj *psLayer, int nVersion) {
#if defined(USE_WMS_SVR) || defined(USE_WFS_SVR) || defined(USE_WCS_SVR) ||    \
    defined(USE_SOS_SVR)

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

  if (msCheckParentPointer(psLayer->map, "map") == MS_FAILURE)
    return NULL;

  sCssParam[0] = '\0';
  if (nVersion > OWS_1_0_0)
    strcpy(sCssParam, "se:SvgParameter");
  else
    strcpy(sCssParam, "CssParameter");

  sNameSpace[0] = '\0';
  if (nVersion > OWS_1_0_0)
    strcpy(sNameSpace, "se:");

  snprintf(szTmp, sizeof(szTmp), "<%sLineSymbolizer>\n", sNameSpace);

  pszSLD = msStringConcatenate(pszSLD, szTmp);

  snprintf(szTmp, sizeof(szTmp), "<%sStroke>\n", sNameSpace);

  pszSLD = msStringConcatenate(pszSLD, szTmp);

  pszGraphicSLD = msSLDGetGraphicSLD(psStyle, psLayer, 0, nVersion);
  if (pszGraphicSLD) {
    snprintf(szTmp, sizeof(szTmp), "<%sGraphicStroke>\n", sNameSpace);

    pszSLD = msStringConcatenate(pszSLD, szTmp);

    pszSLD = msStringConcatenate(pszSLD, pszGraphicSLD);

    if (nVersion >= OWS_1_1_0) {
      if (psStyle->gap > 0) {
        snprintf(szTmp, sizeof(szTmp), "<%sGap>%.2f</%sGap>\n", sNameSpace,
                 psStyle->gap, sNameSpace);
      }
      if (psStyle->initialgap > 0) {
        snprintf(szTmp, sizeof(szTmp), "<%sInitialGap>%.2f</%sInitialGap>\n",
                 sNameSpace, psStyle->initialgap, sNameSpace);
      }
    }

    snprintf(szTmp, sizeof(szTmp), "</%sGraphicStroke>\n", sNameSpace);

    pszSLD = msStringConcatenate(pszSLD, szTmp);

    free(pszGraphicSLD);
    pszGraphicSLD = NULL;
  }

  if (psStyle->color.red != -1 && psStyle->color.green != -1 &&
      psStyle->color.blue != -1)
    sprintf(szHexColor, "%02x%02x%02x", psStyle->color.red,
            psStyle->color.green, psStyle->color.blue);
  else
    sprintf(szHexColor, "%02x%02x%02x", psStyle->outlinecolor.red,
            psStyle->outlinecolor.green, psStyle->outlinecolor.blue);

  snprintf(szTmp, sizeof(szTmp), "<%s name=\"stroke\">#%s</%s>\n", sCssParam,
           szHexColor, sCssParam);
  pszSLD = msStringConcatenate(pszSLD, szTmp);

  if (psStyle->color.alpha != 255 && psStyle->color.alpha != -1) {
    snprintf(szTmp, sizeof(szTmp), "<%s name=\"stroke-opacity\">%.2f</%s>\n",
             sCssParam, (float)psStyle->color.alpha / 255.0, sCssParam);
    pszSLD = msStringConcatenate(pszSLD, szTmp);
  }

  nSymbol = -1;

  if (psStyle->symbol >= 0)
    nSymbol = psStyle->symbol;
  else if (psStyle->symbolname)
    nSymbol = msGetSymbolIndex(&psLayer->map->symbolset, psStyle->symbolname,
                               MS_FALSE);

  if (nSymbol < 0)
    dfSize = 1.0;
  else {
    if (psStyle->size > 0)
      dfSize = psStyle->size;
    else if (psStyle->width > 0)
      dfSize = psStyle->width;
    else
      dfSize = 1;
  }

  snprintf(szTmp, sizeof(szTmp), "<%s name=\"stroke-width\">%.2f</%s>\n",
           sCssParam, dfSize, sCssParam);
  pszSLD = msStringConcatenate(pszSLD, szTmp);

  /* -------------------------------------------------------------------- */
  /*      dash array                                                      */
  /* -------------------------------------------------------------------- */

  if (psStyle->patternlength > 0) {
    for (i = 0; i < psStyle->patternlength; i++) {
      snprintf(szTmp, sizeof(szTmp), "%.2f ", psStyle->pattern[i]);
      pszDashArray = msStringConcatenate(pszDashArray, szTmp);
    }
    // remove the final trailing space from the last pattern value
    msStringTrimBlanks(pszDashArray);

    snprintf(szTmp, sizeof(szTmp), "<%s name=\"stroke-dasharray\">%s</%s>\n",
             sCssParam, pszDashArray, sCssParam);
    pszSLD = msStringConcatenate(pszSLD, szTmp);
    msFree(pszDashArray);
  }

  snprintf(szTmp, sizeof(szTmp), "</%sStroke>\n", sNameSpace);

  pszSLD = msStringConcatenate(pszSLD, szTmp);

  snprintf(szTmp, sizeof(szTmp), "</%sLineSymbolizer>\n", sNameSpace);

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
char *msSLDGeneratePolygonSLD(styleObj *psStyle, layerObj *psLayer,
                              int nVersion) {
#if defined(USE_WMS_SVR) || defined(USE_WFS_SVR) || defined(USE_WCS_SVR) ||    \
    defined(USE_SOS_SVR)

  char szTmp[100];
  char *pszSLD = NULL;
  char szHexColor[7];
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

  snprintf(szTmp, sizeof(szTmp), "<%sPolygonSymbolizer>\n", sNameSpace);

  pszSLD = msStringConcatenate(pszSLD, szTmp);
  /* fill */
  if (psStyle->color.red != -1 && psStyle->color.green != -1 &&
      psStyle->color.blue != -1) {

    snprintf(szTmp, sizeof(szTmp), "<%sFill>\n", sNameSpace);

    pszSLD = msStringConcatenate(pszSLD, szTmp);

    char *pszGraphicSLD = msSLDGetGraphicSLD(psStyle, psLayer, 0, nVersion);
    if (pszGraphicSLD) {
      snprintf(szTmp, sizeof(szTmp), "<%sGraphicFill>\n", sNameSpace);

      pszSLD = msStringConcatenate(pszSLD, szTmp);

      pszSLD = msStringConcatenate(pszSLD, pszGraphicSLD);

      snprintf(szTmp, sizeof(szTmp), "</%sGraphicFill>\n", sNameSpace);

      pszSLD = msStringConcatenate(pszSLD, szTmp);

      free(pszGraphicSLD);
    }

    sprintf(szHexColor, "%02x%02x%02x", psStyle->color.red,
            psStyle->color.green, psStyle->color.blue);

    snprintf(szTmp, sizeof(szTmp), "<%s name=\"fill\">#%s</%s>\n", sCssParam,
             szHexColor, sCssParam);
    pszSLD = msStringConcatenate(pszSLD, szTmp);

    if (psStyle->color.alpha != 255 && psStyle->color.alpha != -1) {
      snprintf(szTmp, sizeof(szTmp), "<%s name=\"fill-opacity\">%.2f</%s>\n",
               sCssParam, (float)psStyle->color.alpha / 255, sCssParam);
      pszSLD = msStringConcatenate(pszSLD, szTmp);
    }

    snprintf(szTmp, sizeof(szTmp), "</%sFill>\n", sNameSpace);

    pszSLD = msStringConcatenate(pszSLD, szTmp);
  }
  /* stroke */
  if (psStyle->outlinecolor.red != -1 && psStyle->outlinecolor.green != -1 &&
      psStyle->outlinecolor.blue != -1) {
    snprintf(szTmp, sizeof(szTmp), "<%sStroke>\n", sNameSpace);
    pszSLD = msStringConcatenate(pszSLD, szTmp);

    /* If there is a symbol to be used for stroke, the color in the */
    /* style should be set to -1. Else It won't apply here. */
    if (psStyle->color.red == -1 && psStyle->color.green == -1 &&
        psStyle->color.blue == -1) {
      char *pszGraphicSLD = msSLDGetGraphicSLD(psStyle, psLayer, 0, nVersion);
      if (pszGraphicSLD) {
        snprintf(szTmp, sizeof(szTmp), "<%sGraphicFill>\n", sNameSpace);
        pszSLD = msStringConcatenate(pszSLD, szTmp);

        pszSLD = msStringConcatenate(pszSLD, pszGraphicSLD);
        snprintf(szTmp, sizeof(szTmp), "</%sGraphicFill>\n", sNameSpace);
        pszSLD = msStringConcatenate(pszSLD, szTmp);

        free(pszGraphicSLD);
      }
    }

    sprintf(szHexColor, "%02x%02x%02x", psStyle->outlinecolor.red,
            psStyle->outlinecolor.green, psStyle->outlinecolor.blue);

    snprintf(szTmp, sizeof(szTmp), "<%s name=\"stroke\">#%s</%s>\n", sCssParam,
             szHexColor, sCssParam);
    pszSLD = msStringConcatenate(pszSLD, szTmp);

    dfSize = 1.0;
    if (psStyle->size > 0)
      dfSize = psStyle->size;
    else if (psStyle->width > 0)
      dfSize = psStyle->width;

    snprintf(szTmp, sizeof(szTmp), "<%s name=\"stroke-width\">%.2f</%s>\n",
             sCssParam, dfSize, sCssParam);
    pszSLD = msStringConcatenate(pszSLD, szTmp);

    if (psStyle->outlinecolor.alpha != 255 &&
        psStyle->outlinecolor.alpha != -1) {
      snprintf(szTmp, sizeof(szTmp), "<%s name=\"stroke-opacity\">%.2f</%s>\n",
               sCssParam, psStyle->outlinecolor.alpha / 255.0, sCssParam);
      pszSLD = msStringConcatenate(pszSLD, szTmp);
    }

    snprintf(szTmp, sizeof(szTmp), "</%sStroke>\n", sNameSpace);
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
char *msSLDGeneratePointSLD(styleObj *psStyle, layerObj *psLayer,
                            int nVersion) {
#if defined(USE_WMS_SVR) || defined(USE_WFS_SVR) || defined(USE_WCS_SVR) ||    \
    defined(USE_SOS_SVR)
  char *pszSLD = NULL;
  char *pszGraphicSLD = NULL;
  char szTmp[100];
  char sNameSpace[10];

  sNameSpace[0] = '\0';
  if (nVersion > OWS_1_0_0)
    strcpy(sNameSpace, "se:");

  snprintf(szTmp, sizeof(szTmp), "<%sPointSymbolizer>\n", sNameSpace);
  pszSLD = msStringConcatenate(pszSLD, szTmp);

  pszGraphicSLD = msSLDGetGraphicSLD(psStyle, psLayer, 1, nVersion);
  if (pszGraphicSLD) {
    pszSLD = msStringConcatenate(pszSLD, pszGraphicSLD);
    free(pszGraphicSLD);
  }

  snprintf(szTmp, sizeof(szTmp), "</%sPointSymbolizer>\n", sNameSpace);
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
char *msSLDGenerateTextSLD(classObj *psClass, layerObj *psLayer, int nVersion) {
#if defined(USE_WMS_SVR) || defined(USE_WFS_SVR) || defined(USE_WCS_SVR) ||    \
    defined(USE_SOS_SVR)
  char *pszSLD = NULL;

  char szTmp[1000];
  char **aszFontsParts = NULL;
  int nFontParts = 0;
  char szHexColor[7];
  double dfAnchorX = 0.5, dfAnchorY = 0.5;
  int i = 0;
  int lid;
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

  if (!psLayer || !psClass)
    return pszSLD;

  for (lid = 0; lid < psClass->numlabels; lid++) {
    char *psLabelText;
    expressionObj psLabelExpr;
    parseObj p;

    msInitExpression(&psLabelExpr);
    psLabelObj = psClass->labels[lid];

    if (psLabelObj->text.string) {
      psLabelExpr.string = msStrdup(psLabelObj->text.string);
      psLabelExpr.type = psLabelObj->text.type;
    } else if (psClass->text.string) {
      psLabelExpr.string = msStrdup(psClass->text.string);
      psLabelExpr.type = psClass->text.type;
    } else if (psLayer->labelitem) {
      psLabelExpr.string = msStrdup(psLayer->labelitem);
      psLabelExpr.type = MS_STRING;
    } else {
      msFreeExpression(&psLabelExpr);
      continue; // Can't find text content for this <Label>
    }

    if (psLabelExpr.type == MS_STRING) {
      // Rewrite string to an expression so that literal strings and attributes
      // are explicitely concatenated, e.g.:
      //   "area is: [area]" becomes ("area is: "+"[area]"+"")
      //             ^^^^^^                     ^^^^^^^^^^^^
      char *result;
      result = msStrdup("\"");
      result = msStringConcatenate(result, psLabelExpr.string);
      result = msStringConcatenate(result, "\"");
      msTokenizeExpression(&psLabelExpr, NULL, NULL);
      for (tokenListNodeObjPtr t = psLabelExpr.tokens; t; t = t->next) {
        if (t->token == MS_TOKEN_BINDING_DOUBLE ||
            t->token == MS_TOKEN_BINDING_INTEGER ||
            t->token == MS_TOKEN_BINDING_STRING) {
          char *target = static_cast<char *>(
              msSmallMalloc(strlen(t->tokenval.bindval.item) + 3));
          char *replacement = static_cast<char *>(
              msSmallMalloc(strlen(t->tokenval.bindval.item) + 9));
          sprintf(target, "[%s]", t->tokenval.bindval.item);
          sprintf(replacement, "\"+\"[%s]\"+\"", t->tokenval.bindval.item);
          result = msReplaceSubstring(result, target, replacement);
          msFree(target);
          msFree(replacement);
        }
      }
      msFreeExpression(&psLabelExpr);
      psLabelExpr.string = msStrdup(result);
      psLabelExpr.type = MS_EXPRESSION;
      msFree(result);
    }

    // Parse label expression to generate SLD tags from MapFile syntax
    msTokenizeExpression(&psLabelExpr, NULL, NULL);
    p.expr = &psLabelExpr;
    p.shape = NULL;
    p.type = MS_PARSE_TYPE_SLD;
    yyparse(&p);
    psLabelText = msStrdup(p.result.strval);
    msFree(p.result.strval);
    msFreeExpression(&psLabelExpr);

    snprintf(szTmp, sizeof(szTmp), "<%sTextSymbolizer>\n", sNameSpace);
    pszSLD = msStringConcatenate(pszSLD, szTmp);

    snprintf(szTmp, sizeof(szTmp), "<%sLabel>\n%s</%sLabel>\n", sNameSpace,
             psLabelText, sNameSpace);
    pszSLD = msStringConcatenate(pszSLD, szTmp);

    /* -------------------------------------------------------------------- */
    /*      only true type fonts are exported. Font name should be          */
    /*      something like arial-bold-italic. There are 3 parts to the      */
    /*      name font-family, font-style (italic, oblique, normal),         */
    /*      font-weight (bold, normal). These 3 elements are separated      */
    /*      with -.                                                         */
    /* -------------------------------------------------------------------- */
    if (psLabelObj->font) {
      aszFontsParts = msStringSplit(psLabelObj->font, '-', &nFontParts);
      if (nFontParts > 0) {
        snprintf(szTmp, sizeof(szTmp), "<%sFont>\n", sNameSpace);
        pszSLD = msStringConcatenate(pszSLD, szTmp);

        /* assuming first one is font-family */
        snprintf(szTmp, sizeof(szTmp), "<%s name=\"font-family\">%s</%s>\n",
                 sCssParam, aszFontsParts[0], sCssParam);
        pszSLD = msStringConcatenate(pszSLD, szTmp);
        for (i = 1; i < nFontParts; i++) {
          if (strcasecmp(aszFontsParts[i], "italic") == 0 ||
              strcasecmp(aszFontsParts[i], "oblique") == 0) {
            snprintf(szTmp, sizeof(szTmp), "<%s name=\"font-style\">%s</%s>\n",
                     sCssParam, aszFontsParts[i], sCssParam);
            pszSLD = msStringConcatenate(pszSLD, szTmp);
          } else if (strcasecmp(aszFontsParts[i], "bold") == 0) {
            snprintf(szTmp, sizeof(szTmp), "<%s name=\"font-weight\">%s</%s>\n",
                     sCssParam, aszFontsParts[i], sCssParam);
            pszSLD = msStringConcatenate(pszSLD, szTmp);
          }
        }
        /* size */
        if (psLabelObj->size > 0) {
          snprintf(szTmp, sizeof(szTmp), "<%s name=\"font-size\">%d</%s>\n",
                   sCssParam, psLabelObj->size, sCssParam);
          pszSLD = msStringConcatenate(pszSLD, szTmp);
        }
        snprintf(szTmp, sizeof(szTmp), "</%sFont>\n", sNameSpace);
        pszSLD = msStringConcatenate(pszSLD, szTmp);
      }
      msFreeCharArray(aszFontsParts, nFontParts);
    }

    /* label placement */
    snprintf(szTmp, sizeof(szTmp), "<%sLabelPlacement>\n<%sPointPlacement>\n",
             sNameSpace, sNameSpace);
    pszSLD = msStringConcatenate(pszSLD, szTmp);

    snprintf(szTmp, sizeof(szTmp), "<%sAnchorPoint>\n", sNameSpace);
    pszSLD = msStringConcatenate(pszSLD, szTmp);

    if (psLabelObj->position == MS_LL) {
      dfAnchorX = 0;
      dfAnchorY = 0;
    } else if (psLabelObj->position == MS_CL) {
      dfAnchorX = 0;
      dfAnchorY = 0.5;
    } else if (psLabelObj->position == MS_UL) {
      dfAnchorX = 0;
      dfAnchorY = 1;
    }

    else if (psLabelObj->position == MS_LC) {
      dfAnchorX = 0.5;
      dfAnchorY = 0;
    } else if (psLabelObj->position == MS_CC) {
      dfAnchorX = 0.5;
      dfAnchorY = 0.5;
    } else if (psLabelObj->position == MS_UC) {
      dfAnchorX = 0.5;
      dfAnchorY = 1;
    }

    else if (psLabelObj->position == MS_LR) {
      dfAnchorX = 1;
      dfAnchorY = 0;
    } else if (psLabelObj->position == MS_CR) {
      dfAnchorX = 1;
      dfAnchorY = 0.5;
    } else if (psLabelObj->position == MS_UR) {
      dfAnchorX = 1;
      dfAnchorY = 1;
    }
    snprintf(szTmp, sizeof(szTmp), "<%sAnchorPointX>%.1f</%sAnchorPointX>\n",
             sNameSpace, dfAnchorX, sNameSpace);
    pszSLD = msStringConcatenate(pszSLD, szTmp);
    snprintf(szTmp, sizeof(szTmp), "<%sAnchorPointY>%.1f</%sAnchorPointY>\n",
             sNameSpace, dfAnchorY, sNameSpace);
    pszSLD = msStringConcatenate(pszSLD, szTmp);

    snprintf(szTmp, sizeof(szTmp), "</%sAnchorPoint>\n", sNameSpace);
    pszSLD = msStringConcatenate(pszSLD, szTmp);

    /* displacement */
    if (psLabelObj->offsetx > 0 || psLabelObj->offsety > 0) {
      snprintf(szTmp, sizeof(szTmp), "<%sDisplacement>\n", sNameSpace);
      pszSLD = msStringConcatenate(pszSLD, szTmp);

      if (psLabelObj->offsetx > 0) {
        snprintf(szTmp, sizeof(szTmp),
                 "<%sDisplacementX>%d</%sDisplacementX>\n", sNameSpace,
                 psLabelObj->offsetx, sNameSpace);
        pszSLD = msStringConcatenate(pszSLD, szTmp);
      }
      if (psLabelObj->offsety > 0) {
        snprintf(szTmp, sizeof(szTmp),
                 "<%sDisplacementY>%d</%sDisplacementY>\n", sNameSpace,
                 psLabelObj->offsety, sNameSpace);
        pszSLD = msStringConcatenate(pszSLD, szTmp);
      }

      snprintf(szTmp, sizeof(szTmp), "</%sDisplacement>\n", sNameSpace);
      pszSLD = msStringConcatenate(pszSLD, szTmp);
    }

    /* rotation */
    if (psLabelObj->angle > 0) {
      snprintf(szTmp, sizeof(szTmp), "<%sRotation>%.2f</%sRotation>\n",
               sNameSpace, psLabelObj->angle, sNameSpace);
      pszSLD = msStringConcatenate(pszSLD, szTmp);
    }

    snprintf(szTmp, sizeof(szTmp), "</%sPointPlacement>\n</%sLabelPlacement>\n",
             sNameSpace, sNameSpace);
    pszSLD = msStringConcatenate(pszSLD, szTmp);

    if (psLabelObj->outlinecolor.red != -1 &&
        psLabelObj->outlinecolor.green != -1 &&
        psLabelObj->outlinecolor.blue != -1) {
      snprintf(szTmp, sizeof(szTmp), "<%sHalo>\n", sNameSpace);
      pszSLD = msStringConcatenate(pszSLD, szTmp);
      snprintf(szTmp, sizeof(szTmp), "<%sRadius>%d</%sRadius>\n", sNameSpace,
               psLabelObj->outlinewidth, sNameSpace);
      pszSLD = msStringConcatenate(pszSLD, szTmp);
      snprintf(szTmp, sizeof(szTmp), "<%sFill>\n", sNameSpace);
      pszSLD = msStringConcatenate(pszSLD, szTmp);
      snprintf(szTmp, sizeof(szTmp), "<%s name=\"fill\">#%02x%02x%02x</%s>\n",
               sCssParam, psLabelObj->outlinecolor.red,
               psLabelObj->outlinecolor.green, psLabelObj->outlinecolor.blue,
               sCssParam);
      pszSLD = msStringConcatenate(pszSLD, szTmp);
      snprintf(szTmp, sizeof(szTmp), "</%sFill>\n", sNameSpace);
      pszSLD = msStringConcatenate(pszSLD, szTmp);
      snprintf(szTmp, sizeof(szTmp), "</%sHalo>\n", sNameSpace);
      pszSLD = msStringConcatenate(pszSLD, szTmp);
    }

    /* color */
    if (psLabelObj->color.red != -1 && psLabelObj->color.green != -1 &&
        psLabelObj->color.blue != -1) {
      snprintf(szTmp, sizeof(szTmp), "<%sFill>\n", sNameSpace);
      pszSLD = msStringConcatenate(pszSLD, szTmp);

      sprintf(szHexColor, "%02hhx%02hhx%02hhx",
              (unsigned char)psLabelObj->color.red,
              (unsigned char)psLabelObj->color.green,
              (unsigned char)psLabelObj->color.blue);

      snprintf(szTmp, sizeof(szTmp), "<%s name=\"fill\">#%s</%s>\n", sCssParam,
               szHexColor, sCssParam);
      pszSLD = msStringConcatenate(pszSLD, szTmp);

      snprintf(szTmp, sizeof(szTmp), "</%sFill>\n", sNameSpace);
      pszSLD = msStringConcatenate(pszSLD, szTmp);
    }

    snprintf(szTmp, sizeof(szTmp), "</%sTextSymbolizer>\n", sNameSpace);
    pszSLD = msStringConcatenate(pszSLD, szTmp);

    msFree(psLabelText);
  }
  return pszSLD;

#else
  return NULL;
#endif
}

#if (defined(USE_WMS_SVR) || defined(USE_WFS_SVR) || defined(USE_WCS_SVR) ||   \
     defined(USE_SOS_SVR))

static void msSLDAppendName(msStringBuffer *sb, const char *pszName,
                            int nVersion) {
  char *pszEncoded = msEncodeHTMLEntities(pszName);
  msStringBufferAppend(sb, (nVersion > OWS_1_0_0) ? "<se:Name>" : "<Name>");
  msStringBufferAppend(sb, pszEncoded);
  msStringBufferAppend(sb,
                       (nVersion > OWS_1_0_0) ? "</se:Name>\n" : "</Name>\n");
  msFree(pszEncoded);
}

static void msSLDGenerateUserStyle(msStringBuffer *sb, layerObj *psLayer,
                                   int nVersion, const char *pszTargetGroup) {
  const char *pszWfsFilter;

  msStringBufferAppend(sb, "<UserStyle>\n");

  if (pszTargetGroup) {
    msSLDAppendName(sb, pszTargetGroup, nVersion);
    if (psLayer->classgroup &&
        strcmp(psLayer->classgroup, pszTargetGroup) == 0) {
      msStringBufferAppend(sb, nVersion > OWS_1_0_0
                                   ? "<se:IsDefault>true</se:IsDefault>\n"
                                   : "<IsDefault>true</IsDefault>\n");
    }
  }

  msStringBufferAppend(sb, nVersion > OWS_1_0_0 ? "<se:FeatureTypeStyle>\n"
                                                : "<FeatureTypeStyle>\n");

  pszWfsFilter = msLookupHashTable(&(psLayer->metadata), "wfs_filter");
  if (psLayer->numclasses > 0) {
    int i;
    for (i = 0; i < psLayer->numclasses; i++) {
      char *pszFilter;
      double dfMinScale = -1, dfMaxScale = -1;

      /* Only write down classes that match the group of interest */
      if (psLayer->_class[i]->group) {
        if (pszTargetGroup == NULL ||
            strcmp(psLayer->_class[i]->group, pszTargetGroup) != 0) {
          continue;
        }
      } else if (pszTargetGroup != NULL) {
        continue;
      }

      msStringBufferAppend(sb,
                           nVersion > OWS_1_0_0 ? "<se:Rule>\n" : "<Rule>\n");

      /* if class has a name, use it as the RULE name */
      if (psLayer->_class[i]->name) {
        msSLDAppendName(sb, psLayer->_class[i]->name, nVersion);
      }
      /* -------------------------------------------------------------------- */
      /*      get the Filter if there is a class expression.                  */
      /* -------------------------------------------------------------------- */
      pszFilter = msSLDGetFilter(psLayer->_class[i], pszWfsFilter);

      if (pszFilter) {
        msStringBufferAppend(sb, pszFilter);
        free(pszFilter);
      }
      /* -------------------------------------------------------------------- */
      /*      generate the min/max scale.                                     */
      /* -------------------------------------------------------------------- */
      dfMinScale = -1.0;
      if (psLayer->_class[i]->minscaledenom > 0)
        dfMinScale = psLayer->_class[i]->minscaledenom;
      else if (psLayer->minscaledenom > 0)
        dfMinScale = psLayer->minscaledenom;
      else if (psLayer->map && psLayer->map->web.minscaledenom > 0)
        dfMinScale = psLayer->map->web.minscaledenom;
      if (dfMinScale > 0) {
        char szTmp[100];
        if (nVersion > OWS_1_0_0)
          snprintf(szTmp, sizeof(szTmp),
                   "<se:MinScaleDenominator>%f</se:MinScaleDenominator>\n",
                   dfMinScale);
        else
          snprintf(szTmp, sizeof(szTmp),
                   "<MinScaleDenominator>%f</MinScaleDenominator>\n",
                   dfMinScale);

        msStringBufferAppend(sb, szTmp);
      }

      dfMaxScale = -1.0;
      if (psLayer->_class[i]->maxscaledenom > 0)
        dfMaxScale = psLayer->_class[i]->maxscaledenom;
      else if (psLayer->maxscaledenom > 0)
        dfMaxScale = psLayer->maxscaledenom;
      else if (psLayer->map && psLayer->map->web.maxscaledenom > 0)
        dfMaxScale = psLayer->map->web.maxscaledenom;
      if (dfMaxScale > 0) {
        char szTmp[100];
        if (nVersion > OWS_1_0_0)
          snprintf(szTmp, sizeof(szTmp),
                   "<se:MaxScaleDenominator>%f</se:MaxScaleDenominator>\n",
                   dfMaxScale);
        else
          snprintf(szTmp, sizeof(szTmp),
                   "<MaxScaleDenominator>%f</MaxScaleDenominator>\n",
                   dfMaxScale);

        msStringBufferAppend(sb, szTmp);
      }

      /* -------------------------------------------------------------------- */
      /*      Line symbolizer.                                                */
      /*                                                                      */
      /*      Right now only generates a stroke element containing css        */
      /*      parameters.                                                     */
      /*      Lines using symbols TODO (specially for dash lines)             */
      /* -------------------------------------------------------------------- */
      if (psLayer->type == MS_LAYER_LINE) {
        int j;
        for (j = 0; j < psLayer->_class[i]->numstyles; j++) {
          styleObj *psStyle = psLayer->_class[i]->styles[j];
          char *pszSLD = msSLDGenerateLineSLD(psStyle, psLayer, nVersion);
          if (pszSLD) {
            msStringBufferAppend(sb, pszSLD);
            free(pszSLD);
          }
        }

      } else if (psLayer->type == MS_LAYER_POLYGON) {
        int j;
        for (j = 0; j < psLayer->_class[i]->numstyles; j++) {
          styleObj *psStyle = psLayer->_class[i]->styles[j];
          char *pszSLD = msSLDGeneratePolygonSLD(psStyle, psLayer, nVersion);
          if (pszSLD) {
            msStringBufferAppend(sb, pszSLD);
            free(pszSLD);
          }
        }

      } else if (psLayer->type == MS_LAYER_POINT) {
        int j;
        for (j = 0; j < psLayer->_class[i]->numstyles; j++) {
          styleObj *psStyle = psLayer->_class[i]->styles[j];
          char *pszSLD = msSLDGeneratePointSLD(psStyle, psLayer, nVersion);
          if (pszSLD) {
            msStringBufferAppend(sb, pszSLD);
            free(pszSLD);
          }
        }
      }
      {
        /* label if it exists */
        char *pszSLD =
            msSLDGenerateTextSLD(psLayer->_class[i], psLayer, nVersion);
        if (pszSLD) {
          msStringBufferAppend(sb, pszSLD);
          free(pszSLD);
        }
      }
      msStringBufferAppend(sb,
                           nVersion > OWS_1_0_0 ? "</se:Rule>\n" : "</Rule>\n");
    }
  }

  msStringBufferAppend(sb, nVersion > OWS_1_0_0 ? "</se:FeatureTypeStyle>\n"
                                                : "</FeatureTypeStyle>\n");
  msStringBufferAppend(sb, "</UserStyle>\n");
}

#endif

/************************************************************************/
/*                          msSLDGenerateSLDLayer                       */
/*                                                                      */
/*      Generate an SLD XML string based on the layer's classes.        */
/************************************************************************/
char *msSLDGenerateSLDLayer(layerObj *psLayer, int nVersion) {
#if defined(USE_WMS_SVR) || defined(USE_WFS_SVR) || defined(USE_WCS_SVR) ||    \
    defined(USE_SOS_SVR)

  const char *pszWMSLayerName = NULL;
  msStringBuffer *sb = msStringBufferAlloc();

  if (psLayer && (psLayer->status == MS_ON || psLayer->status == MS_DEFAULT) &&
      (psLayer->type == MS_LAYER_POINT || psLayer->type == MS_LAYER_LINE ||
       psLayer->type == MS_LAYER_POLYGON)) {

    int i;
    int numClassGroupNames = 0;
    char **papszClassGroupNames =
        (char **)msSmallMalloc(sizeof(char *) * psLayer->numclasses);
    for (i = 0; i < psLayer->numclasses; i++) {
      const char *group = psLayer->_class[i]->group;
      int j;
      for (j = 0; j < numClassGroupNames; j++) {
        if (group == NULL) {
          if (papszClassGroupNames[j] == NULL)
            break;
        } else if (papszClassGroupNames[j] != NULL &&
                   strcmp(papszClassGroupNames[j], group) == 0) {
          break;
        }
      }
      if (j >= numClassGroupNames) {
        papszClassGroupNames[numClassGroupNames] =
            group ? msStrdup(group) : NULL;
        numClassGroupNames++;
      }
    }

    msStringBufferAppend(sb, "<NamedLayer>\n");

    pszWMSLayerName = msOWSLookupMetadata(&(psLayer->metadata), "MO", "name");
    msSLDAppendName(sb,
                    pszWMSLayerName ? pszWMSLayerName
                    : psLayer->name ? psLayer->name
                                    : "NamedLayer",
                    nVersion);

    for (i = 0; i < numClassGroupNames; i++) {
      msSLDGenerateUserStyle(sb, psLayer, nVersion, papszClassGroupNames[i]);
      msFree(papszClassGroupNames[i]);
    }
    msFree(papszClassGroupNames);

    msStringBufferAppend(sb, "</NamedLayer>\n");
  }
  return msStringBufferReleaseStringAndFree(sb);

#else
  msSetError(MS_MISCERR, "OWS support is not available.",
             "msSLDGenerateSLDLayer()");
  return NULL;
#endif
}

static const char *msSLDGetComparisonValue(const char *pszExpression) {
  if (!pszExpression)
    return NULL;

  if (strstr(pszExpression, "<=") || strstr(pszExpression, " le "))
    return "PropertyIsLessThanOrEqualTo";
  else if (strstr(pszExpression, "=~"))
    return "PropertyIsLike";
  else if (strstr(pszExpression, "~*"))
    return "PropertyIsLike";
  else if (strstr(pszExpression, ">=") || strstr(pszExpression, " ge "))
    return "PropertyIsGreaterThanOrEqualTo";
  else if (strstr(pszExpression, "!=") || strstr(pszExpression, " ne "))
    return "PropertyIsNotEqualTo";
  else if (strstr(pszExpression, "=") || strstr(pszExpression, " eq "))
    return "PropertyIsEqualTo";
  else if (strstr(pszExpression, "<") || strstr(pszExpression, " lt "))
    return "PropertyIsLessThan";
  else if (strstr(pszExpression, ">") || strstr(pszExpression, " gt "))
    return "PropertyIsGreaterThan";

  return NULL;
}

static const char *msSLDGetLogicalOperator(const char *pszExpression) {

  if (!pszExpression)
    return NULL;

  if (strcasestr(pszExpression, " AND ") || strcasestr(pszExpression, "AND("))
    return "And";

  if (strcasestr(pszExpression, " OR ") || strcasestr(pszExpression, "OR("))
    return "Or";

  if (strcasestr(pszExpression, "NOT ") || strcasestr(pszExpression, "NOT("))
    return "Not";

  return NULL;
}

static char *msSLDGetRightExpressionOfOperator(const char *pszExpression) {
  const char *pszAnd = NULL, *pszOr = NULL, *pszNot = NULL;

  pszAnd = strcasestr(pszExpression, " AND ");

  if (!pszAnd) {
    pszAnd = strcasestr(pszExpression, "AND(");
  }

  if (pszAnd)
    return msStrdup(pszAnd + 4);
  else {
    pszOr = strcasestr(pszExpression, " OR ");

    if (!pszOr) {
      pszOr = strcasestr(pszExpression, "OR(");
    }

    if (pszOr)
      return msStrdup(pszOr + 3);
    else {
      pszNot = strcasestr(pszExpression, "NOT ");

      if (!pszNot)
        pszNot = strcasestr(pszExpression, "NOT(");

      if (pszNot)
        return msStrdup(pszNot + 4);
    }
  }
  return NULL;
}

static char *msSLDGetLeftExpressionOfOperator(const char *pszExpression) {
  char *pszReturn = NULL;
  int nLength = 0, iReturn = 0;

  if (!pszExpression || (nLength = strlen(pszExpression)) <= 0)
    return NULL;

  pszReturn = (char *)malloc(sizeof(char) * (nLength + 1));
  pszReturn[0] = '\0';
  if (strcasestr(pszExpression, " AND ")) {
    for (int i = 0; i < nLength - 5; i++) {
      if (pszExpression[i] == ' ' && (toupper(pszExpression[i + 1]) == 'A') &&
          (toupper(pszExpression[i + 2]) == 'N') &&
          (toupper(pszExpression[i + 3]) == 'D') &&
          (pszExpression[i + 4] == ' '))
        break;
      else {
        pszReturn[iReturn++] = pszExpression[i];
      }
      pszReturn[iReturn] = '\0';
    }
  } else if (strcasestr(pszExpression, "AND(")) {
    for (int i = 0; i < nLength - 4; i++) {
      if ((toupper(pszExpression[i]) == 'A') &&
          (toupper(pszExpression[i + 1]) == 'N') &&
          (toupper(pszExpression[i + 2]) == 'D') &&
          (pszExpression[i + 3] == '('))
        break;
      else {
        pszReturn[iReturn++] = pszExpression[i];
      }
      pszReturn[iReturn] = '\0';
    }
  } else if (strcasestr(pszExpression, " OR ")) {
    for (int i = 0; i < nLength - 4; i++) {
      if (pszExpression[i] == ' ' && (toupper(pszExpression[i + 1]) == 'O') &&
          (toupper(pszExpression[i + 2]) == 'R') && pszExpression[i + 3] == ' ')
        break;
      else {
        pszReturn[iReturn++] = pszExpression[i];
      }
      pszReturn[iReturn] = '\0';
    }
  } else if (strcasestr(pszExpression, "OR(")) {
    for (int i = 0; i < nLength - 3; i++) {
      if ((toupper(pszExpression[i]) == 'O') &&
          (toupper(pszExpression[i + 1]) == 'R') && pszExpression[i + 2] == '(')
        break;
      else {
        pszReturn[iReturn++] = pszExpression[i];
      }
      pszReturn[iReturn] = '\0';
    }
  } else {
    msFree(pszReturn);
    return NULL;
  }

  return pszReturn;
}

static int msSLDNumberOfLogicalOperators(const char *pszExpression) {
  const char *pszAnd = NULL;
  const char *pszOr = NULL;
  const char *pszNot = NULL;
  const char *pszSecondAnd = NULL, *pszSecondOr = NULL;
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

  if (!pszNot) {
    pszNot = strcasestr(pszExpression, "NOT(");
  }

  if (!pszAnd && !pszOr && !pszNot)
    return 0;

  /* doen not matter how many exactly if there are 2 or more */
  if ((pszAnd && pszOr) || (pszAnd && pszNot) || (pszOr && pszNot))
    return 2;

  if (pszAnd) {
    pszSecondAnd = strcasestr(pszAnd + 3, " AND ");
    pszSecondOr = strcasestr(pszAnd + 3, " OR ");
  } else if (pszOr) {
    pszSecondAnd = strcasestr(pszOr + 2, " AND ");
    pszSecondOr = strcasestr(pszOr + 2, " OR ");
  }

  if (!pszSecondAnd && !pszSecondOr)
    return 1;
  else
    return 2;
}

static char *msSLDGetAttributeNameOrValue(const char *pszExpression,
                                          const char *pszComparionValue,
                                          int bReturnName) {
  char **aszValues = NULL;
  char *pszAttributeName = NULL;
  char *pszAttributeValue = NULL;
  char cCompare = '=';
  char szCompare[3] = {0};
  char szCompare2[3] = {0};
  int bOneCharCompare = -1, nTokens = 0, nLength = 0;
  int iValue = 0, i = 0, iValueIndex = 0;
  int bStartCopy = 0, iAtt = 0;
  char *pszFinalAttributeName = NULL, *pszFinalAttributeValue = NULL;
  int bSingleQuote = 0, bDoubleQuote = 0;

  if (!pszExpression || !pszComparionValue || strlen(pszExpression) == 0)
    return NULL;

  szCompare[0] = '\0';
  szCompare2[0] = '\0';

  if (strcasecmp(pszComparionValue, "PropertyIsEqualTo") == 0) {
    cCompare = '=';
    szCompare[0] = 'e';
    szCompare[1] = 'q';
    szCompare[2] = '\0';

    bOneCharCompare = 1;
  }
  if (strcasecmp(pszComparionValue, "PropertyIsNotEqualTo") == 0) {
    szCompare[0] = 'n';
    szCompare[1] = 'e';
    szCompare[2] = '\0';

    szCompare2[0] = '!';
    szCompare2[1] = '=';
    szCompare2[2] = '\0';

    bOneCharCompare = 0;
  } else if (strcasecmp(pszComparionValue, "PropertyIsLike") == 0) {
    szCompare[0] = '=';
    szCompare[1] = '~';
    szCompare[2] = '\0';

    szCompare2[0] = '~';
    szCompare2[1] = '*';
    szCompare2[2] = '\0';

    bOneCharCompare = 0;
  } else if (strcasecmp(pszComparionValue, "PropertyIsLessThan") == 0) {
    cCompare = '<';
    szCompare[0] = 'l';
    szCompare[1] = 't';
    szCompare[2] = '\0';
    bOneCharCompare = 1;
  } else if (strcasecmp(pszComparionValue, "PropertyIsLessThanOrEqualTo") ==
             0) {
    szCompare[0] = 'l';
    szCompare[1] = 'e';
    szCompare[2] = '\0';

    szCompare2[0] = '<';
    szCompare2[1] = '=';
    szCompare2[2] = '\0';

    bOneCharCompare = 0;
  } else if (strcasecmp(pszComparionValue, "PropertyIsGreaterThan") == 0) {
    cCompare = '>';
    szCompare[0] = 'g';
    szCompare[1] = 't';
    szCompare[2] = '\0';
    bOneCharCompare = 1;
  } else if (strcasecmp(pszComparionValue, "PropertyIsGreaterThanOrEqualTo") ==
             0) {
    szCompare[0] = 'g';
    szCompare[1] = 'e';
    szCompare[2] = '\0';

    szCompare2[0] = '>';
    szCompare2[1] = '=';
    szCompare2[2] = '\0';

    bOneCharCompare = 0;
  }

  if (bOneCharCompare == 1) {
    aszValues = msStringSplit(pszExpression, cCompare, &nTokens);
    if (nTokens > 1) {
      pszAttributeName = msStrdup(aszValues[0]);
      pszAttributeValue = msStrdup(aszValues[1]);
    } else {
      nLength = strlen(pszExpression);
      pszAttributeName = (char *)malloc(sizeof(char) * (nLength + 1));
      iValue = 0;
      for (i = 0; i < nLength - 2; i++) {
        if (pszExpression[i] != szCompare[0] &&
            pszExpression[i] != toupper(szCompare[0])) {
          pszAttributeName[iValue++] = pszExpression[i];
        } else {
          if ((pszExpression[i + 1] == szCompare[1] ||
               pszExpression[i + 1] == toupper(szCompare[1])) &&
              (pszExpression[i + 2] == ' ')) {
            iValueIndex = i + 3;
            pszAttributeValue = msStrdup(pszExpression + iValueIndex);
            break;
          } else
            pszAttributeName[iValue++] = pszExpression[i];
        }
      }
      pszAttributeName[iValue] = '\0';
    }
    msFreeCharArray(aszValues, nTokens);
  } else if (bOneCharCompare == 0) {
    nLength = strlen(pszExpression);
    pszAttributeName = (char *)malloc(sizeof(char) * (nLength + 1));
    iValue = 0;
    for (i = 0; i < nLength - 2; i++) {
      if ((pszExpression[i] != szCompare[0] ||
           pszExpression[i] != toupper(szCompare[0])) &&
          (pszExpression[i] != szCompare2[0] ||
           pszExpression[i] != toupper(szCompare2[0])))

      {
        pszAttributeName[iValue++] = pszExpression[i];
      } else {
        if (((pszExpression[i + 1] == szCompare[1] ||
              pszExpression[i + 1] == toupper(szCompare[1])) ||
             (pszExpression[i + 1] == szCompare2[1] ||
              pszExpression[i + 1] == toupper(szCompare2[1]))) &&
            (pszExpression[i + 2] == ' ')) {
          iValueIndex = i + 3;
          pszAttributeValue = msStrdup(pszExpression + iValueIndex);
          break;
        } else
          pszAttributeName[iValue++] = pszExpression[i];
      }
    }
    pszAttributeName[iValue] = '\0';
  }

  /* -------------------------------------------------------------------- */
  /*      Return the name of the attribute : It is supposed to be         */
  /*      inside []                                                       */
  /* -------------------------------------------------------------------- */
  if (bReturnName) {
    if (!pszAttributeName) {
      msFree(pszAttributeValue);
      return NULL;
    }

    nLength = strlen(pszAttributeName);
    pszFinalAttributeName = (char *)malloc(sizeof(char) * (nLength + 1));
    bStartCopy = 0;
    iAtt = 0;
    for (i = 0; i < nLength; i++) {
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

    msFree(pszAttributeName);
    msFree(pszAttributeValue);
    return pszFinalAttributeName;
  } else {

    if (!pszAttributeValue) {
      msFree(pszAttributeName);
      return NULL;
    }
    nLength = strlen(pszAttributeValue);
    pszFinalAttributeValue = (char *)malloc(sizeof(char) * (nLength + 1));
    pszFinalAttributeValue[0] = '\0';
    bStartCopy = 0;
    iAtt = 0;
    for (i = 0; i < nLength; i++) {
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
        bStartCopy = 1;

      if (bStartCopy) {
        if (pszAttributeValue[i] == '\'' && bSingleQuote)
          break;
        else if (pszAttributeValue[i] == '"' && bDoubleQuote)
          break;
        else if (pszAttributeValue[i] == ')') {
          // remove any spaces prior to the closing bracket
          msStringTrimBlanks(pszFinalAttributeValue);
          break;
        }
        pszFinalAttributeValue[iAtt++] = pszAttributeValue[i];
      }
      pszFinalAttributeValue[iAtt] = '\0';
    }

    /*trim  for regular expressions*/
    if (strlen(pszFinalAttributeValue) > 2 &&
        strcasecmp(pszComparionValue, "PropertyIsLike") == 0) {
      int len = strlen(pszFinalAttributeValue);
      msStringTrimBlanks(pszFinalAttributeValue);
      if (pszFinalAttributeValue[0] == '/' &&
          (pszFinalAttributeValue[len - 1] == '/' ||
           (pszFinalAttributeValue[len - 1] == 'i' &&
            pszFinalAttributeValue[len - 2] == '/'))) {
        if (pszFinalAttributeValue[len - 1] == '/')
          pszFinalAttributeValue[len - 1] = '\0';
        else
          pszFinalAttributeValue[len - 2] = '\0';

        memmove(pszFinalAttributeValue,
                pszFinalAttributeValue +
                    ((pszFinalAttributeValue[1] == '^') ? 2 : 1),
                len - 1);

        /*replace wild card string .* with * */
        pszFinalAttributeValue =
            msReplaceSubstring(pszFinalAttributeValue, ".*", "*");
      }
    }
    msFree(pszAttributeName);
    msFree(pszAttributeValue);
    return pszFinalAttributeValue;
  }
}

static char *msSLDGetAttributeName(const char *pszExpression,
                                   const char *pszComparionValue) {
  return msSLDGetAttributeNameOrValue(pszExpression, pszComparionValue, 1);
}

static char *msSLDGetAttributeValue(const char *pszExpression,
                                    const char *pszComparionValue) {
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
static FilterEncodingNode *BuildExpressionTree(const char *pszExpression,
                                               FilterEncodingNode *psNode) {
  int nOperators = 0;

  if (!pszExpression || strlen(pszExpression) == 0)
    return NULL;

  /* -------------------------------------------------------------------- */
  /*      First we check how many logical operators are there :           */
  /*       - if none : It means It is a comparison operator (like =,      */
  /*      >, >= .... We get the comparison value as well as the           */
  /*      attribute and the attribute's value and assign it to the node   */
  /*      passed in argument.                                             */
  /*       - if there is one operator, we assign the operator to the      */
  /*      node and adds the expressions into the left and right nodes.    */
  /* -------------------------------------------------------------------- */
  nOperators = msSLDNumberOfLogicalOperators(pszExpression);
  if (nOperators == 0) {
    if (!psNode)
      psNode = FLTCreateFilterEncodingNode();

    const char *pszComparionValue = msSLDGetComparisonValue(pszExpression);
    char *pszAttibuteName =
        msSLDGetAttributeName(pszExpression, pszComparionValue);
    char *pszAttibuteValue =
        msSLDGetAttributeValue(pszExpression, pszComparionValue);
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
    }
    free(pszAttibuteName);
    free(pszAttibuteValue);
    return psNode;

  } else if (nOperators == 1) {
    const char *pszOperator = msSLDGetLogicalOperator(pszExpression);
    if (pszOperator) {
      if (!psNode)
        psNode = FLTCreateFilterEncodingNode();

      psNode->eType = FILTER_NODE_TYPE_LOGICAL;
      psNode->pszValue = msStrdup(pszOperator);

      char *pszLeftExpression = msSLDGetLeftExpressionOfOperator(pszExpression);
      char *pszRightExpression =
          msSLDGetRightExpressionOfOperator(pszExpression);

      if (pszLeftExpression || pszRightExpression) {
        if (pszLeftExpression) {
          const char *pszComparionValue =
              msSLDGetComparisonValue(pszLeftExpression);
          char *pszAttibuteName =
              msSLDGetAttributeName(pszLeftExpression, pszComparionValue);
          char *pszAttibuteValue =
              msSLDGetAttributeValue(pszLeftExpression, pszComparionValue);

          if (pszComparionValue && pszAttibuteName && pszAttibuteValue) {
            psNode->psLeftNode = FLTCreateFilterEncodingNode();
            psNode->psLeftNode->eType = FILTER_NODE_TYPE_COMPARISON;
            psNode->psLeftNode->pszValue = msStrdup(pszComparionValue);

            psNode->psLeftNode->psLeftNode = FLTCreateFilterEncodingNode();
            psNode->psLeftNode->psLeftNode->eType =
                FILTER_NODE_TYPE_PROPERTYNAME;
            psNode->psLeftNode->psLeftNode->pszValue =
                msStrdup(pszAttibuteName);

            psNode->psLeftNode->psRightNode = FLTCreateFilterEncodingNode();
            psNode->psLeftNode->psRightNode->eType = FILTER_NODE_TYPE_LITERAL;
            psNode->psLeftNode->psRightNode->pszValue =
                msStrdup(pszAttibuteValue);
          }

          free(pszAttibuteName);
          free(pszAttibuteValue);
          free(pszLeftExpression);
        }
        if (pszRightExpression) {
          const char *pszComparionValue =
              msSLDGetComparisonValue(pszRightExpression);
          char *pszAttibuteName =
              msSLDGetAttributeName(pszRightExpression, pszComparionValue);
          char *pszAttibuteValue =
              msSLDGetAttributeValue(pszRightExpression, pszComparionValue);

          if (pszComparionValue && pszAttibuteName && pszAttibuteValue) {
            psNode->psRightNode = FLTCreateFilterEncodingNode();
            psNode->psRightNode->eType = FILTER_NODE_TYPE_COMPARISON;
            psNode->psRightNode->pszValue = msStrdup(pszComparionValue);

            psNode->psRightNode->psLeftNode = FLTCreateFilterEncodingNode();
            psNode->psRightNode->psLeftNode->eType =
                FILTER_NODE_TYPE_PROPERTYNAME;
            psNode->psRightNode->psLeftNode->pszValue =
                msStrdup(pszAttibuteName);

            psNode->psRightNode->psRightNode = FLTCreateFilterEncodingNode();
            psNode->psRightNode->psRightNode->eType = FILTER_NODE_TYPE_LITERAL;
            psNode->psRightNode->psRightNode->pszValue =
                msStrdup(pszAttibuteValue);
          }
          free(pszAttibuteName);
          free(pszAttibuteValue);
          free(pszRightExpression);
        }
      }
    }

    return psNode;
  } else {
    return NULL;
  }
}

static char *msSLDBuildFilterEncoding(const FilterEncodingNode *psNode) {
  char *pszTmp = NULL;
  char szTmp[200];
  char *pszExpression = NULL;

  if (!psNode)
    return NULL;

  if (psNode->eType == FILTER_NODE_TYPE_COMPARISON && psNode->pszValue &&
      psNode->psLeftNode && psNode->psLeftNode->pszValue &&
      psNode->psRightNode && psNode->psRightNode->pszValue) {
    snprintf(szTmp, sizeof(szTmp),
             "<ogc:%s><ogc:PropertyName>%s</ogc:PropertyName><ogc:Literal>%s</"
             "ogc:Literal></ogc:%s>",
             psNode->pszValue, psNode->psLeftNode->pszValue,
             psNode->psRightNode->pszValue, psNode->pszValue);
    pszExpression = msStrdup(szTmp);
  } else if (psNode->eType == FILTER_NODE_TYPE_LOGICAL && psNode->pszValue &&
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
    snprintf(szTmp, sizeof(szTmp), "</ogc:%s>", psNode->pszValue);
    pszExpression = msStringConcatenate(pszExpression, szTmp);
  }
  return pszExpression;
}

static char *msSLDParseLogicalExpression(const char *pszExpression,
                                         const char *pszWfsFilter) {
  FilterEncodingNode *psNode = NULL;
  char *pszFLTExpression = NULL;
  char *pszTmp = NULL;

  if (!pszExpression || strlen(pszExpression) == 0)
    return NULL;

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
    FLTFreeFilterEncodingNode(psNode);
  }

  return pszFLTExpression;
}

/************************************************************************/
/*                     msSLDConvertRegexExpToOgcIsLike                  */
/*                                                                      */
/*      Convert mapserver regex expression to ogc is like property      */
/*      expression.                                                     */
/*                                                                      */
/*      Review bug 1644 for details. Here are the current rules:        */
/*                                                                      */
/*       The filter encoding property like is more limited compared     */
/*      to regular expression that can be built in mapserver. I         */
/*      think we should define what is possible to convert properly     */
/*      and do those, and also identify potential problems.  Example :  */
/*        - any time there is a .* in the expression it will be         */
/*      converted to *                                                  */
/*        - any other character plus all the metacharacters . ^ $ * +   */
/*      ? { [ ] \ | ( ) would be outputted as is. (In case of           */
/*      mapserver, when we read the the ogc filter expression, we       */
/*      convert the wild card character to .*, and we convert the       */
/*      single character to .  and the escape character to \ all        */
/*      other are outputted as is)                                      */
/*        - the  ogc tag would look like <ogc:PropertyIsLike            */
/*      wildCard="*"  singleChar="." escape="\">                        */
/*                                                                      */
/*        - type of potential problem :                                 */
/*           * if an expression is like /T (star)/ it will be           */
/*      converted to T* which is not correct.                           */
/*                                                                      */
/************************************************************************/

static char *msSLDConvertRegexExpToOgcIsLike(const char *pszRegex) {
  char szBuffer[1024];
  int iBuffer = 0, i = 0;
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
      if (i < nLength - 1 && pszRegex[i + 1] == '*') {
        szBuffer[iBuffer++] = '*';
        i = i + 2;
      } else {
        szBuffer[iBuffer++] = pszRegex[i];
        i++;
      }
    }
  }
  szBuffer[iBuffer] = '\0';

  return msStrdup(szBuffer);
}

/************************************************************************/
/*                          XMLEscape                                   */
/************************************************************************/
static std::string XMLEscape(const char *pszValue) {
  char *pszEscaped = CPLEscapeString(pszValue, -1, CPLES_XML);
  std::string osRet(pszEscaped);
  CPLFree(pszEscaped);
  return osRet;
}

static std::string GetPropertyIsEqualTo(const char *pszPropertyName,
                                        const char *pszLiteral) {
  std::string osFilter("<ogc:PropertyIsEqualTo><ogc:PropertyName>");
  osFilter += XMLEscape(pszPropertyName);
  osFilter += "</ogc:PropertyName><ogc:Literal>";
  osFilter += XMLEscape(pszLiteral);
  osFilter += "</ogc:Literal></ogc:PropertyIsEqualTo>";
  return osFilter;
}

static std::string GetPropertyIsLike(const char *pszPropertyName,
                                     const char *pszLiteral) {
  std::string osFilter("<ogc:PropertyIsLike wildCard=\"*\" singleChar=\".\" "
                       "escape=\"\\\"><ogc:PropertyName>");
  osFilter += XMLEscape(pszPropertyName);
  osFilter += "</ogc:PropertyName><ogc:Literal>";
  osFilter += XMLEscape(pszLiteral);
  osFilter += "</ogc:Literal></ogc:PropertyIsLike>";
  return osFilter;
}

/************************************************************************/
/*                              msSLDGetFilter                          */
/*                                                                      */
/*      Get the corresponding ogc Filter based on the class             */
/*      expression. TODO : move function to mapogcfilter.c when         */
/*      finished.                                                       */
/************************************************************************/
char *msSLDGetFilter(classObj *psClass, const char *pszWfsFilter) {
  char *pszFilter = NULL;
  char *pszOgcFilter = NULL;

  if (psClass && psClass->expression.string) {

    char *pszExpression = msStrdup(psClass->expression.string);

    /* string expression */
    if (psClass->expression.type == MS_STRING) {

      // ensure any trailing whitespace is removed
      msStringTrimBlanks(pszExpression);
      if (psClass->layer && psClass->layer->classitem) {
        std::string osFilter("<ogc:Filter>");
        if (pszWfsFilter) {
          osFilter += "<ogc:And>";
          osFilter += pszWfsFilter;
        }
        osFilter +=
            GetPropertyIsEqualTo(psClass->layer->classitem, pszExpression);
        if (pszWfsFilter) {
          osFilter += "</ogc:And>";
        }
        osFilter += "</ogc:Filter>\n";
        pszFilter = msStrdup(osFilter.c_str());
      }
    } else if (psClass->expression.type == MS_EXPRESSION) {
      msStringTrimBlanks(pszExpression);
      pszFilter = msSLDParseLogicalExpression(pszExpression, pszWfsFilter);
    } else if (psClass->expression.type == MS_LIST) {
      if (psClass->layer && psClass->layer->classitem) {

        char **listExpressionValues = NULL;
        int numListExpressionValues = 0;
        int i = 0;
        int tokenCount = 0;
        std::string osOrFilters;

        listExpressionValues =
            msStringSplit(pszExpression, ',', &numListExpressionValues);

        // loop through all values in the list and create a PropertyIsEqualTo
        // for each value
        for (i = 0; i < numListExpressionValues; i++) {
          if (listExpressionValues[i] && listExpressionValues[i][0] != '\0') {
            osOrFilters += GetPropertyIsEqualTo(psClass->layer->classitem,
                                                listExpressionValues[i]);
            osOrFilters += '\n';
            tokenCount++;
          }
        }

        std::string osFilter("<ogc:Filter>");

        // no need for an OR clause if there is only one item in the list
        if (tokenCount == 1) {
          osFilter += osOrFilters;
        } else if (tokenCount > 1) {
          osFilter += "<ogc:Or>";
          osFilter += osOrFilters;
          osFilter += "</ogc:Or>";
        }
        osFilter += "</ogc:Filter>";

        // don't filter when the list is empty
        if (tokenCount > 0) {
          pszFilter = msStrdup(osFilter.c_str());
        }
        msFreeCharArray(listExpressionValues, numListExpressionValues);
      }
    } else if (psClass->expression.type == MS_REGEX) {
      if (psClass->layer && psClass->layer->classitem) {
        pszOgcFilter =
            msSLDConvertRegexExpToOgcIsLike(psClass->expression.string);

        std::string osFilter("<ogc:Filter>");
        if (pszWfsFilter) {
          osFilter += "<ogc:And>";
          osFilter += pszWfsFilter;
        }
        osFilter += GetPropertyIsLike(psClass->layer->classitem, pszOgcFilter);

        free(pszOgcFilter);

        if (pszWfsFilter) {
          osFilter += "</ogc:And>";
        }
        osFilter += "</ogc:Filter>\n";
        pszFilter = msStrdup(osFilter.c_str());
      }
    }

    msFree(pszExpression);
  } else if (pszWfsFilter) {
    std::string osFilter("<ogc:Filter>");
    osFilter += pszWfsFilter;
    osFilter += "</ogc:Filter>\n";
    pszFilter = msStrdup(osFilter.c_str());
  }
  return pszFilter;
}

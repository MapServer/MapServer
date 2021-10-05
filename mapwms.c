/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  OpenGIS Web Mapping Service support implementation.
 * Author:   Steve Lime and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2005 Regents of the University of Minnesota.
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

#define _GNU_SOURCE

#include "mapserver.h"
#include "maperror.h"
#include "mapthread.h"
#include "mapgml.h"
#include <ctype.h>
#include "maptemplate.h"
#include "mapows.h"

#include "mapogcsld.h"
#include "mapogcfilter.h"
#include "mapowscommon.h"

#include "maptime.h"
#include "mapproject.h"

#include <stdarg.h>
#include <time.h>
#include <string.h>

#ifdef _WIN32
#include <process.h>
#endif

/* ==================================================================
 * WMS Server stuff.
 * ================================================================== */
#ifdef USE_WMS_SVR

/*
** msWMSException()
**
** Report current MapServer error in requested format.
*/

int msWMSException(mapObj *map, int nVersion, const char *exception_code,
                   char *wms_exception_format)
{
  char *schemalocation = NULL;

  /* Default to WMS 1.3.0 exceptions if version not set yet */
  if (nVersion <= 0)
    nVersion = OWS_1_3_0;

  /* get scheam location */
  schemalocation = msEncodeHTMLEntities( msOWSGetSchemasLocation(map) );

  /* Establish default exception format depending on VERSION */
  if (wms_exception_format == NULL) {
    if (nVersion <= OWS_1_0_0)
      wms_exception_format = "INIMAGE";  /* WMS 1.0.0 */
    else if (nVersion <= OWS_1_0_7)
      wms_exception_format = "SE_XML";   /* WMS 1.0.1 to 1.0.7 */
    else if (nVersion <= OWS_1_1_1)
      wms_exception_format = "application/vnd.ogc.se_xml"; /* WMS 1.1.0 and later */
    else
      wms_exception_format = "text/xml";
  }

  if (strcasecmp(wms_exception_format, "INIMAGE") == 0 ||
      strcasecmp(wms_exception_format, "BLANK") == 0 ||
      strcasecmp(wms_exception_format, "application/vnd.ogc.se_inimage")== 0 ||
      strcasecmp(wms_exception_format, "application/vnd.ogc.se_blank") == 0) {
    int blank = 0;

    if (strcasecmp(wms_exception_format, "BLANK") == 0 ||
        strcasecmp(wms_exception_format, "application/vnd.ogc.se_blank") == 0) {
      blank = 1;
    }

    msWriteErrorImage(map, NULL, blank);

  } else if (strcasecmp(wms_exception_format, "WMS_XML") == 0) { /* Only in V1.0.0 */
    msIO_setHeader("Content-Type","text/xml; charset=UTF-8");
    msIO_sendHeaders();

    msIO_printf("<WMTException version=\"1.0.0\">\n");
    msWriteErrorXML(stdout);
    msIO_printf("</WMTException>\n");
  } else /* XML error, the default: SE_XML (1.0.1 to 1.0.7) */
    /* or application/vnd.ogc.se_xml (1.1.0 and later) */
  {
    if (nVersion <= OWS_1_0_7) {
      /* In V1.0.1 to 1.0.7, the MIME type was text/xml */
      msIO_setHeader("Content-Type","text/xml; charset=UTF-8");
      msIO_sendHeaders();

      msIO_printf("<?xml version='1.0' encoding=\"UTF-8\" standalone=\"no\" ?>\n");
      msIO_printf("<!DOCTYPE ServiceExceptionReport SYSTEM \"http://www.digitalearth.gov/wmt/xml/exception_1_0_1.dtd\">\n");

      msIO_printf("<ServiceExceptionReport version=\"1.0.1\">\n");
    } else if (nVersion <= OWS_1_1_0) {
      /* In V1.1.0 and later, we have OGC-specific MIME types */
      /* we cannot return anything else than application/vnd.ogc.se_xml here. */
      msIO_setHeader("Content-Type","application/vnd.ogc.se_xml; charset=UTF-8");
      msIO_sendHeaders();

      msIO_printf("<?xml version='1.0' encoding=\"UTF-8\" standalone=\"no\" ?>\n");

      msIO_printf("<!DOCTYPE ServiceExceptionReport SYSTEM \"%s/wms/1.1.0/exception_1_1_0.dtd\">\n",schemalocation);

      msIO_printf("<ServiceExceptionReport version=\"1.1.0\">\n");
    } else if (nVersion <= OWS_1_1_1) { /* 1.1.1 */
      msIO_setHeader("Content-Type","application/vnd.ogc.se_xml; charset=UTF-8");
      msIO_sendHeaders();

      msIO_printf("<?xml version='1.0' encoding=\"UTF-8\" standalone=\"no\" ?>\n");
      msIO_printf("<!DOCTYPE ServiceExceptionReport SYSTEM \"%s/wms/1.1.1/exception_1_1_1.dtd\">\n", schemalocation);
      msIO_printf("<ServiceExceptionReport version=\"1.1.1\">\n");
    } else { /*1.3.0*/
      if (strcasecmp(wms_exception_format, "application/vnd.ogc.se_xml") == 0) {
        msIO_setHeader("Content-Type","application/vnd.ogc.se_xml; charset=UTF-8");
      } else {
        msIO_setHeader("Content-Type","text/xml; charset=UTF-8");
      }
      msIO_sendHeaders();

      msIO_printf("<?xml version='1.0' encoding=\"UTF-8\" standalone=\"no\" ?>\n");
      msIO_printf("<ServiceExceptionReport version=\"1.3.0\" xmlns=\"http://www.opengis.net/ogc\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.opengis.net/ogc %s/wms/1.3.0/exceptions_1_3_0.xsd\">\n",
                  schemalocation);
    }


    if (exception_code)
      msIO_printf("<ServiceException code=\"%s\">\n", exception_code);
    else
      msIO_printf("<ServiceException>\n");
    msWriteErrorXML(stdout);
    msIO_printf("</ServiceException>\n");
    msIO_printf("</ServiceExceptionReport>\n");

  }
  free(schemalocation);

  return MS_FAILURE; /* so that we can call 'return msWMSException();' anywhere */
}

int msWMSSetTimePattern(const char *timepatternstring, char *timestring, int checkonly)
{
  int ret = MS_SUCCESS;

  if (timepatternstring && timestring) {
    /* parse the time parameter to extract a distinct time. */
    /* time value can be dicrete times (eg 2004-09-21), */
    /* multiple times (2004-09-21, 2004-09-22, ...) */
    /* and range(s) (2004-09-21/2004-09-25, 2004-09-27/2004-09-29) */
    int numtimes = 0;
    char** atimes = msStringSplit(timestring, ',', &numtimes);
    
    /* get the pattern to use */
    if (numtimes > 0) {
      int numpatterns = 0;
      char** patterns = msStringSplit(timepatternstring, ',', &numpatterns);

      for (int j=0; j<numtimes;++j) {
        int numranges = 0;
        char** ranges = msStringSplit(atimes[j],  '/', &numranges);
        for (int k=0; k<numranges;++k) {
          int match = MS_FALSE;
          for (int i=0; i<numpatterns; ++i) {
            if (patterns[i] && strlen(patterns[i]) > 0) {
              msStringTrimBlanks(patterns[i]);
              char* tmpstr = msStringTrimLeft(patterns[i]);
              if (msTimeMatchPattern(ranges[k], tmpstr) == MS_TRUE) {
                if (!checkonly) msSetLimitedPatternsToUse(tmpstr);
                match = MS_TRUE;
                break;
              }
            }
          }
          if (match == MS_FALSE) {
            msSetError(MS_WMSERR, "Time value %s given does not match the time format pattern.", "msWMSSetTimePattern", ranges[k]);
            ret = MS_FAILURE;
            break;
          }
        }
         msFreeCharArray(ranges, numranges);
         if (ret == MS_FAILURE)
           break;
      }

      msFreeCharArray(patterns, numpatterns);
    }
    msFreeCharArray(atimes, numtimes);              
  }

  return ret;
}

/*
** Apply the TIME parameter to layers that are time aware
*/
int msWMSApplyTime(mapObj *map, int version, char *time, char *wms_exception_format)
{
  int i=0;
  layerObj *lp = NULL;
  const char *timeextent, *timefield, *timedefault, *timepattern = NULL;
    
  if (map) {

    timepattern = msOWSLookupMetadata(&(map->web.metadata), "MO", "timeformat");

    for (i=0; i<map->numlayers; i++) {
      lp = (GET_LAYER(map, i));
      if (lp->status != MS_ON && lp->status != MS_DEFAULT)
        continue;

      /* check if the layer is time aware */
      timeextent = msOWSLookupMetadata(&(lp->metadata), "MO", "timeextent");
      timefield =  msOWSLookupMetadata(&(lp->metadata), "MO", "timeitem");
      timedefault =  msOWSLookupMetadata(&(lp->metadata), "MO", "timedefault");

      if (timeextent && timefield) {
        /* check to see if the time value is given. If not */
        /* use default time. If default time is not available */
        /* send an exception */
        if (time == NULL || strlen(time) <=0) {
          if (timedefault == NULL) {
            msSetError(MS_WMSERR, "No Time value was given, and no default time value defined.", "msWMSApplyTime");
            return msWMSException(map, version, "MissingDimensionValue", wms_exception_format);
          } else {
            if (msValidateTimeValue((char *)timedefault, timeextent) == MS_FALSE) {
              msSetError(MS_WMSERR, "No Time value was given, and the default time value %s is invalid or outside the time extent defined %s", "msWMSApplyTime", timedefault, timeextent);
              /* return MS_FALSE; */
              return msWMSException(map, version, "InvalidDimensionValue", wms_exception_format);
            }
            msLayerSetTimeFilter(lp, timedefault, timefield);
          }
        } else {
          /* 
	        ** Check to see if there is a list of possible patterns defined. If it is the case, use 
          ** it to set the time pattern to use for the request.
          **
          ** Last argument is set to TRUE (checkonly) to not trigger the patterns info setting, rather 
          ** to only apply the wms_timeformats on the user request values, not the mapfile values. 
          */
          if (timepattern && time && strlen(time) > 0) {
            if (msWMSSetTimePattern(timepattern, time, MS_TRUE) == MS_FAILURE) 
              return msWMSException(map, version,"InvalidDimensionValue", wms_exception_format);
          }
          
          /* check if given time is in the range */
          if (msValidateTimeValue(time, timeextent) == MS_FALSE) {
            if (timedefault == NULL) {
              msSetError(MS_WMSERR, "Time value(s) %s given is invalid or outside the time extent defined (%s).", "msWMSApplyTime", time, timeextent);
              /* return MS_FALSE; */
              return msWMSException(map, version,
                                    "InvalidDimensionValue", wms_exception_format);
            } else {
              if (msValidateTimeValue((char *)timedefault, timeextent) == MS_FALSE) {
                msSetError(MS_WMSERR, "Time value(s) %s given is invalid or outside the time extent defined (%s), and default time set is invalid (%s)", "msWMSApplyTime", time, timeextent, timedefault);
                /* return MS_FALSE; */
                return msWMSException(map, version,
                                      "InvalidDimensionValue",  wms_exception_format);
              } else
                msLayerSetTimeFilter(lp, timedefault, timefield);
            }

          } else {
            /* build the time string */
            msLayerSetTimeFilter(lp, time, timefield);
            timeextent= NULL;
          }
        }
      }
    }

    /* last argument is MS_FALSE to trigger a method call that set the patterns
       info. some drivers use it */
    if (timepattern && time && strlen(time) > 0) {
      if (msWMSSetTimePattern(timepattern, time, MS_FALSE) == MS_FAILURE)
        return msWMSException(map, version, "InvalidDimensionValue", wms_exception_format);
    }    
  }

  return MS_SUCCESS;
}

/*
** Apply the FILTER parameter to layers (RFC118)
*/
int msWMSApplyFilter(mapObj *map, int version, const char *filter,
		     int def_srs_needs_axis_swap, char *wms_exception_format, owsRequestObj *ows_request)
{
  int i=0, numlayers;
  int numfilters=0, curfilter=0;
  char **paszFilters = NULL;
  FilterEncodingNode *psNode = NULL;

  // Empty filter should be ignored
  if (!filter || strlen(filter) == 0)
    return MS_SUCCESS;

  if (!map)
    return MS_FAILURE;  

  /* Count number of requested layers / groups / etc.
   * Only layers with STATUS ON were in the LAYERS request param.
   * Layers with STATUS DEFAULT were set in the mapfile and are
   * not expected to have a corresponding filter in the request
   */
  for(i=0, numlayers=0; i<map->numlayers; i++) {
    layerObj *lp=NULL;

    if(map->layerorder[i] != -1) {
      lp = (GET_LAYER(map,  map->layerorder[i]));
      if (lp->status == MS_ON)
        numlayers++;
    }
  }
   
  /* -------------------------------------------------------------------- */
  /*      Parse the Filter parameter. If there are several Filter         */
  /*      parameters, each Filter is inside parentheses.                  */
  /* -------------------------------------------------------------------- */
  numfilters = 0;
  if (filter[0] == '(') {
    paszFilters = FLTSplitFilters(filter, &numfilters);

  } else if (numlayers == 1) {
    numfilters=1;
    paszFilters = (char **)msSmallMalloc(sizeof(char *)*numfilters);
    paszFilters[0] = msStrdup(filter);
  }

  if (numfilters != ows_request->numwmslayerargs) {
    msSetError(MS_WMSERR, "Wrong number of filter elements, one filter must be specified for each requested layer or groups.",
	       "msWMSApplyFilter" );
    msFreeCharArray(paszFilters, numfilters);
    return msWMSException(map, version, "InvalidParameterValue", wms_exception_format);
  }

  /* We're good to go. Apply each filter to the corresponding layer */
  for(i=0; i<map->numlayers; i++) {
    layerObj *lp=NULL;

    if(map->layerorder[i] != -1)
      lp = (GET_LAYER(map,  map->layerorder[i]));

    /* Only layers with STATUS ON were in the LAYERS request param.*/
    if (lp == NULL || lp->status != MS_ON)
      continue;

    curfilter = ows_request->layerwmsfilterindex[lp->index];

    /* Skip empty filters */
    if (paszFilters[curfilter][0] == '\0') {
      continue;
    }
    
    /* Force setting a template to enable query. */
    if (lp->template == NULL)
      lp->template = msStrdup("ttt.html");

    /* Parse filter */
    psNode = FLTParseFilterEncoding(paszFilters[curfilter]);
    if (!psNode) {
      msSetError(MS_WMSERR,
		 "Invalid or Unsupported FILTER : %s",
		 "msWMSApplyFilter()", paszFilters[curfilter]);
      msFreeCharArray(paszFilters, numfilters);
      return msWMSException(map, version, "InvalidParameterValue", wms_exception_format);
    }

    /* For WMS 1.3 and up, we may need to swap the axis of bbox and geometry
     * elements inside the filter(s)
     */
    if (version >= OWS_1_3_0)
      FLTDoAxisSwappingIfNecessary(map, psNode, def_srs_needs_axis_swap);

#ifdef do_we_need_this
    FLTProcessPropertyIsNull(psNode, map, lp->index);

    /*preparse the filter for gml aliases*/
    FLTPreParseFilterForAliasAndGroup(psNode, map, lp->index, "G");

    /* Check that FeatureId filters are consistent with the active layer */
    if( FLTCheckFeatureIdFilters(psNode, map, lp->index) == MS_FAILURE)
      {
        FLTFreeFilterEncodingNode( psNode );
        return msWFSException(map, "mapserv", MS_OWS_ERROR_NO_APPLICABLE_CODE, paramsObj->pszVersion);
      }

    /* FIXME?: could probably apply to WFS 1.1 too */
    if( nWFSVersion >= OWS_2_0_0 )
      {
	int nEvaluation;

        if( FLTCheckInvalidOperand(psNode) == MS_FAILURE)
        {
            FLTFreeFilterEncodingNode( psNode );
            msFreeCharArray(paszFilters, numfilters);
            return msWFSException(map, "filter", MS_WFS_ERROR_OPERATION_PROCESSING_FAILED, paramsObj->pszVersion);
        }

        if( FLTCheckInvalidProperty(psNode, map, lp->index) == MS_FAILURE)
        {
            FLTFreeFilterEncodingNode( psNode );
            msFreeCharArray(paszFilters, numfilters);
            return msWFSException(map, "filter", MS_OWS_ERROR_INVALID_PARAMETER_VALUE, paramsObj->pszVersion);
        }

        psNode = FLTSimplify(psNode, &nEvaluation);
        if( psNode == NULL )
        {
            FLTFreeFilterEncodingNode( psNode );
            msFreeCharArray(paszFilters, numfilters);
            if( nEvaluation == 1 ) {
                /* return full layer */
                return msWFSRunBasicGetFeature(map, lp, paramsObj, nWFSVersion);
            }
            else {
                /* return empty result set */
                return MS_SUCCESS;
            }
        }

      }
    
#endif

    /* Apply filter to this layer */

    /* But first, start by removing any wfs_use_default_extent_for_getfeature metadata item */
    /* that could result in the BBOX to be removed */
    char* old_value_wfs_use_default_extent_for_getfeature = NULL;
    {
        const char* old_value_tmp = msLookupHashTable(
            &(lp->metadata), "wfs_use_default_extent_for_getfeature");
        if( old_value_tmp )
        {
            old_value_wfs_use_default_extent_for_getfeature = msStrdup(old_value_tmp);
            msRemoveHashTable(&(lp->metadata), "wfs_use_default_extent_for_getfeature");
        }
    }

    int ret = FLTApplyFilterToLayer(psNode, map, lp->index);
    if( old_value_wfs_use_default_extent_for_getfeature )
    {
        msInsertHashTable(&(lp->metadata), "wfs_use_default_extent_for_getfeature",
                          old_value_wfs_use_default_extent_for_getfeature);
        msFree(old_value_wfs_use_default_extent_for_getfeature);
    }

    if( ret != MS_SUCCESS ) {
      errorObj* ms_error = msGetErrorObj();

      if(ms_error->code != MS_NOTFOUND) {
	msSetError(MS_WMSERR, "FLTApplyFilterToLayer() failed", "msWMSApplyFilter()");
	FLTFreeFilterEncodingNode( psNode );
        msFreeCharArray(paszFilters, numfilters);
	return msWMSException(map, version, "InvalidParameterValue", wms_exception_format);
      }
    }

    FLTFreeFilterEncodingNode( psNode );

    curfilter++;

  }/* for */

    msFreeCharArray(paszFilters, numfilters);

    return MS_SUCCESS;
}

/*
** msWMSPrepareNestedGroups()
**
** purpose: Parse WMS_LAYER_GROUP settings into arrays
**
** params:
** - nestedGroups: This array holds the arrays of groups that have been set
**                 through the WMS_LAYER_GROUP metadata
** - numNestedGroups: This array holds the number of groups set in
**                    WMS_LAYER_GROUP for each layer
** - isUsedInNestedGroup: This array indicates if the layer is used as group
**                        as set through the WMS_LAYER_GROUP metadata
*/
void msWMSPrepareNestedGroups(mapObj* map, int nVersion, char*** nestedGroups, int* numNestedGroups, int* isUsedInNestedGroup)
{
  int i, k;
  const char* groups;
  char* errorMsg;
  //Create array to hold unique groups
  int maxgroups = 2000;
  int maxgroupiter = 1;
  char** uniqgroups = msSmallMalloc(maxgroups * sizeof(char*));
  int uniqgroupcount = 0;
  

  for (i = 0; i < map->numlayers; i++) {
    nestedGroups[i] = NULL; /* default */
    numNestedGroups[i] = 0; /* default */
    isUsedInNestedGroup[i] = 0; /* default */

    groups = msOWSLookupMetadata(&(GET_LAYER(map, i)->metadata), "MO", "layer_group");
    if ((groups != NULL) && (strlen(groups) != 0)) {
      if (GET_LAYER(map, i)->group != NULL && strlen(GET_LAYER(map, i)->group) != 0) {
        errorMsg = "It is not allowed to set both the GROUP and WMS_LAYER_GROUP for a layer";
        msSetError(MS_WMSERR, errorMsg, "msWMSPrepareNestedGroups()", NULL);
        msIO_fprintf(stdout, "<!-- ERROR: %s -->\n", errorMsg);
        /* cannot return exception at this point because we are already writing to stdout */
      } else {
        if (groups[0] != '/') {
          errorMsg = "The WMS_LAYER_GROUP metadata does not start with a '/'";
          msSetError(MS_WMSERR, errorMsg, "msWMSPrepareNestedGroups()", NULL);
          msIO_fprintf(stdout, "<!-- ERROR: %s -->\n", errorMsg);
          /* cannot return exception at this point because we are already writing to stdout */
        } else {
          /* split into subgroups. Start at address + 1 because the first '/' would cause an extra empty group */
          nestedGroups[i] = msStringSplit(groups + 1, '/', &numNestedGroups[i]);
          /* Iterate through the groups and add them to the unique groups array */
          for (k=0; k<numNestedGroups[i]; k++) {
            int found ,l = 0;
            found = 0;
            for (l=0; l<uniqgroupcount; l++) {
              if ( strcasecmp(uniqgroups[l], nestedGroups[i][k]) == 0 ){
                found = 1;
                break;
              }
            }
            if(found == 0){
             uniqgroups[uniqgroupcount] = nestedGroups[i][k];
             uniqgroupcount++;
             // Does need only when maximum unique groups exceed 2000
             if ( uniqgroupcount == (maxgroups*maxgroupiter)){
                uniqgroups = realloc(uniqgroups, (uniqgroupcount + maxgroups) * sizeof(char*));
                maxgroupiter++;
             }
            }
         }
       } 
     }
   }
 } 
 /* Iterate through layers to find out whether they are in any of the nested groups */
 for (i = 0; i < map->numlayers; i++) {
    for (k=0; k<uniqgroupcount; k++) {
       if ( strcasecmp(GET_LAYER(map, i)->name ,uniqgroups[k]) == 0 ){
             isUsedInNestedGroup[i] = 1;
             break;
        }
     }
  }

  /* free uniqgroups */
  free(uniqgroups);
}


/*
** Validate that a given dimension is inside the extents defined
*/
int msWMSValidateDimensionValue(char *value, const char *dimensionextent, int forcecharcter)
{
  char **extents=NULL, **ranges=NULL, **onerange=NULL;
  int numextents=0, numranges=0, numonerange=0;
  char **aextentvalues = NULL;
  int nextentvalues=0;
  pointObj *aextentranges=NULL;
  int nextentranges=0;

  int isextentavalue = MS_FALSE, isextentarange=MS_FALSE;
  int i,j,status=MS_FALSE;
  char **uservalues=NULL;
  int numuservalues=0;
  int ischaracter = MS_FALSE;
  float minval, maxval,currentval,mincurrentval,maxcurrentval, mincurrentrange, maxcurrentrange;

  int uservaluevalid= MS_FALSE;

  if (forcecharcter)
    ischaracter = MS_TRUE;

  if (!value || !dimensionextent)
    return MS_FALSE;

  /*for the value, we support descrete values (2005) */
  /* multiple values (abc, def, ...) */
  /* and range(s) (1000/2000, 3000/5000) */
  /** we do not support resolution*/

  /* -------------------------------------------------------------------- */
  /*      parse the extent first.                                         */
  /* -------------------------------------------------------------------- */
  extents = msStringSplit (dimensionextent, ',', &numextents);
  if (numextents == 1) {
    if (strstr(dimensionextent, "/") == NULL) {
      /*single value*/
      isextentavalue = MS_TRUE;
      nextentvalues = 1;
      aextentvalues = (char **)msSmallMalloc(sizeof(char *));
      msStringTrim(extents[0]);
      aextentvalues[0] = msStrdup(dimensionextent);
      if (!forcecharcter)
        ischaracter= !FLTIsNumeric((char *)dimensionextent);

    } else {
      ranges = msStringSplit (dimensionextent, '/', &numranges);
      if(ranges && (numranges == 2 || numranges == 3)) {
        /*single range*/
        isextentarange = MS_TRUE;
        aextentranges = msSmallMalloc(sizeof(pointObj));
        nextentranges=1;
        aextentranges[0].x = atof(ranges[0]);
        aextentranges[0].y = atof(ranges[1]);
        /*ranges should be numeric*/
        ischaracter = MS_FALSE;
      }
      msFreeCharArray(ranges, numranges);
      ranges = NULL;
    }
  } else if (numextents > 1) { /*check if it is muliple values or mutliple ranges*/
    if (strstr(dimensionextent, "/") == NULL) {
      /*multiple values*/
      isextentavalue = MS_TRUE;
      aextentvalues = (char **)msSmallMalloc(sizeof(char *)*numextents);
      for (i=0; i<numextents; i++) {
        char *stmp = msStrdup(extents[i]);
        msStringTrim(stmp);
        aextentvalues[i] = stmp;
      }
      nextentvalues = numextents;
      if (!forcecharcter)
        ischaracter= !FLTIsNumeric(aextentvalues[0]);
    } else { /*multiple range extent*/
      int isvalidextent = MS_TRUE;
      /*ranges should be numeric*/
      ischaracter = MS_FALSE;
      isextentarange = MS_TRUE;
      aextentranges = msSmallMalloc(sizeof(pointObj) * numextents);
      nextentranges=0;

      for (i=0; i<numextents; i++) {
        onerange = msStringSplit(extents[i], '/', &numonerange);
        if (!onerange || (numonerange != 2 && numonerange != 3)) {
          isvalidextent = MS_FALSE;
          break;
        }
        if (isvalidextent) {

          aextentranges[nextentranges].x = atof(onerange[0]);
          aextentranges[nextentranges++].y = atof(onerange[1]);
        }
        msFreeCharArray(onerange, numonerange);
        onerange = NULL;
      }
      if (!isvalidextent) {
        msFree(aextentranges);
        aextentranges = NULL;
        nextentranges = 0;
        isextentarange = MS_FALSE;
      }
    }
  }

  if (numextents > 0) {
    msFreeCharArray(extents, numextents);
    extents = NULL;
  }




  /* make sure that we got a valid extent*/
  if (!isextentavalue && !isextentarange) {
    status = MS_FALSE;
    goto dimension_cleanup;
  }

  /*for the extent of the dimesion, we support
  single value,  or list of mulitiple values comma separated,
  a single range or multipe ranges */


  uservalues = msStringSplit (value, ',', &numuservalues);
  uservaluevalid = MS_FALSE;
  if (numuservalues == 1) {
    /*user iput=single*/
    /*is it descret or range*/
    ranges = msStringSplit(uservalues[0], '/', &numranges);
    if (numranges == 1) { /*discrete*/
      if (isextentavalue) {
        /*single user value, single/multiple values extent*/
        for (i=0; i<nextentvalues; i++) {
          if (ischaracter)
            uservaluevalid = !strcmp(uservalues[0], aextentvalues[i]);
          else {
            if (atof(uservalues[0]) == atof(aextentvalues[i]))
              uservaluevalid = MS_TRUE;
          }
          if(uservaluevalid)
            break;
        }
      } else if (isextentarange) {
        /*single user value, single/multiple range extent*/
        currentval = atof(uservalues[0]);

        for (i=0; i<nextentranges; i++) {
          minval = aextentranges[i].x;
          maxval = aextentranges[i].y;
          if (currentval >= minval && currentval <= maxval) {
            uservaluevalid= MS_TRUE;
            break;
          }
        }
      }
    } else if (numranges == 2 || numranges == 3) { /*range*/
      /*user input=single range. In this case the extents must
       be of a range type.*/
      mincurrentval = atof(ranges[0]);
      maxcurrentval = atof(ranges[1]);
      if (isextentarange) {
        for (i=0; i<nextentranges; i++) {
          minval = aextentranges[i].x;
          maxval = aextentranges[i].y;

          if(minval <= mincurrentval && maxval >= maxcurrentval &&
              minval <= maxval) {
            uservaluevalid= MS_TRUE;
            break;
          }
        }
      }
    }
    msFreeCharArray(ranges, numranges);
    ranges = NULL;
  } else if (numuservalues > 1) { /*user input=multiple*/
    if (strstr(value, "/") == NULL) {
      /*user input=multiple value*/
      int valueisvalid = MS_FALSE;
      for (i=0; i<numuservalues; i++) {
        valueisvalid = MS_FALSE;
        if (isextentavalue) {
          /*user input is multiple values, extent is defned as one or multiple values*/
          for (j=0; j<nextentvalues; j++) {
            if (ischaracter) {
              if (strcmp(uservalues[i], aextentvalues[j]) == 0) {
                valueisvalid = MS_TRUE;
                break;
              }
            } else {
              if (atof(uservalues[i]) == atof(aextentvalues[j])) {
                valueisvalid = MS_TRUE;
                break;
              }
            }
          }
          /*every value shoule be valid*/
          if (!valueisvalid)
            break;
        } else if (isextentarange) {
          /*user input is multiple values, extent is defned as one or multiple ranges*/
          for (j=0; j<nextentranges; j++) {
            minval = aextentranges[j].x;
            maxval = aextentranges[j].y;
            currentval = atof(uservalues[i]);
            if(minval <= currentval && maxval >= currentval &&
                minval <= maxval) {
              valueisvalid = MS_TRUE;
              break;
            }
          }
          if (!valueisvalid)
            break;
        }
      }
      uservaluevalid = valueisvalid;
    } else { /*user input multiple ranges*/
      int valueisvalid = MS_TRUE;

      for (i=0; i<numuservalues; i++) {
        /*each ranges should be valid*/
        onerange = msStringSplit(uservalues[i], '/', &numonerange);
        if (numonerange == 2 || numonerange == 3) {
          mincurrentval = atof(onerange[0]);
          maxcurrentval = atof(onerange[1]);

          /*extent must be defined also as a rangle*/
          if (isextentarange) {
            for (j=0; j<nextentranges; j++) {
              mincurrentrange = aextentranges[j].x;
              maxcurrentrange = aextentranges[j].y;

              if(mincurrentval >=mincurrentrange && maxcurrentval <= maxcurrentrange &&
                  mincurrentval <= maxcurrentval) {
                break;
              }

            }
            if (j == nextentranges) {
              valueisvalid = MS_FALSE;
              msFreeCharArray(onerange, numonerange);
              onerange = NULL;
              break;
            }
          }
        } else {
          valueisvalid = MS_FALSE;
        }
        msFreeCharArray(onerange, numonerange);
        onerange = NULL;
      }
      uservaluevalid = valueisvalid;
    }

  }
  if(uservaluevalid)
    status = MS_TRUE;

dimension_cleanup:
  if(ranges)
    msFreeCharArray(ranges, numranges);
  if(onerange)
    msFreeCharArray(onerange, numonerange);
  if(extents)
    msFreeCharArray(extents, numextents);
  if(uservalues)
    msFreeCharArray(uservalues, numuservalues);

  if (aextentvalues) {
    for (i=0; i<nextentvalues; i++)
      msFree(aextentvalues[i]);
    msFree(aextentvalues);
  }

  if(aextentranges) {
    msFree(aextentranges);
  }

  return status;
}


int msWMSApplyDimensionLayer(layerObj *lp, const char *item, char *value, int forcecharcter)
{
  int result = MS_FALSE;
  char *pszExpression=NULL;
  int tlpindex = -1;

  if (lp && item && value) {
    /*for the value, we support descrete values (2005) */
    /* multiple values (abc, def, ...) */
    /* and range(s) (1000/2000, 3000/5000) */
    pszExpression = FLTGetExpressionForValuesRanges(lp, (char *)item, value,  forcecharcter);

    if (pszExpression) {
      // If tileindex is set, the filter is applied to tileindex too.
      if (lp->tileindex && (tlpindex = msGetLayerIndex(lp->map, lp->tileindex)) != -1) {
          if(FLTApplyExpressionToLayer((GET_LAYER(lp->map, tlpindex)), pszExpression))
            result = MS_TRUE;
          else
            result = MS_FALSE;
      }
      if(FLTApplyExpressionToLayer(lp, pszExpression))
        result = MS_TRUE;
      else
        result = MS_FALSE;
      msFree(pszExpression);
    }
  }
  return result;
}


int msWMSApplyDimension(layerObj *lp, int version, char *dimensionname, char *value,
                        char *wms_exception_format)
{
  char *dimensionitemname=NULL, *dimensionextentname=NULL, *dimensiontypename=NULL;
  char *dimensionunitname=NULL, *dimensiondefaultname=NULL;
  const char *dimensionitem, *dimensionextent, *dimensiontype, *dimensionunit, *dimensiondefault;
  int forcecharcter;
  int result = MS_FALSE;
  char *dimension = NULL, *currentvalue=NULL;

  if (lp && dimensionname && value) {
    /*check if the dimension name passes starts with dim_. All dimensions should start with dim_, except elevation*/
    if (strncasecmp(dimensionname, "dim_", 4) == 0)
      dimension = msStrdup(dimensionname+4);
    else
      dimension = msStrdup(dimensionname);

    /*if value is empty and a default is defined, use it*/
    if (strlen(value) > 0)
      currentvalue = msStrdup(value);
    else {
      dimensiondefaultname = msStrdup(dimension);
      dimensiondefaultname = msStringConcatenate(dimensiondefaultname, "_default");
      dimensiondefault = msOWSLookupMetadata(&(lp->metadata), "M", dimensiondefaultname);
      if (dimensiondefault && strlen(dimensiondefault) > 0)
        currentvalue = msStrdup(dimensiondefault);
    }

    /*check if the manadatory metada related to the dimension are set*/
    dimensionitemname = msStrdup(dimension);
    dimensionitemname = msStringConcatenate(dimensionitemname, "_item");
    dimensionitem = msOWSLookupMetadata(&(lp->metadata), "M", dimensionitemname);

    dimensionextentname = msStrdup(dimension);
    dimensionextentname = msStringConcatenate(dimensionextentname, "_extent");
    dimensionextent = msOWSLookupMetadata(&(lp->metadata), "M", dimensionextentname);

    dimensionunitname = msStrdup(dimension);
    dimensionunitname = msStringConcatenate(dimensionunitname, "_units");
    dimensionunit = msOWSLookupMetadata(&(lp->metadata), "M", dimensionunitname);

    /*if the server want to force the type to character*/
    dimensiontypename = msStrdup(dimension);
    dimensiontypename = msStringConcatenate(dimensiontypename, "_type");
    dimensiontype = msOWSLookupMetadata(&(lp->metadata), "M", dimensiontypename);
    forcecharcter = MS_FALSE;
    if (dimensiontype && strcasecmp(dimensiontype, "Character") == 0)
      forcecharcter = MS_TRUE;

    if (dimensionitem && dimensionextent && dimensionunit && currentvalue) {
      if(msWMSValidateDimensionValue(currentvalue, dimensionextent, forcecharcter)) {
        result = msWMSApplyDimensionLayer(lp, dimensionitem, currentvalue, forcecharcter);
      } else {
        msSetError(MS_WMSERR, "Dimension %s with a value of %s is invalid or outside the extents defined", "msWMSApplyDimension",
                   dimension, currentvalue);
        result =  MS_FALSE;
      }
    } else
      msSetError(MS_WMSERR, "Dimension %s : invalid settings. Make sure that item, units and extent are set.", "msWMSApplyDimension",
                 dimension);

    msFree(dimensionitemname);
    msFree(dimensionextentname);
    msFree(dimensiontypename);
    msFree(dimensionunitname);
    msFree(dimensiondefaultname);
    msFree(dimension);
    msFree(currentvalue);
  }
  return result;
}
/*
**
*/
int msWMSLoadGetMapParams(mapObj *map, int nVersion,
                          char **names, char **values, int numentries, char *wms_exception_format,
                          const char *wms_request, owsRequestObj *ows_request)
{
  int i, adjust_extent = MS_FALSE, nonsquare_enabled = MS_FALSE;
  int transparent = MS_NOOVERRIDE;
  int bbox_pixel_is_point = MS_FALSE;
  outputFormatObj *format = NULL;
  int validlayers = 0;
  char *styles = NULL;
  int numwmslayerargs = 0;
  char **layers = NULL;
  int layerfound = MS_FALSE;
  int invalidlayers = 0;
  char epsgbuf[100];
  char srsbuffer[100];
  int epsgvalid = MS_FALSE;
  int timerequest = 0;
  char *stime = NULL;
  char **tokens=NULL;
  int n,j;
  int srsfound = 0;
  int bboxfound = 0;
  int formatfound = 0;
  int widthfound = 0;
  int heightfound = 0;
  char *request = NULL;
  int status = 0;
  const char *layerlimit = NULL;

  const char *sldenabled=NULL;
  char *sld_url=NULL, *sld_body=NULL;

  int need_axis_swap = MS_FALSE;
  const char *filter = NULL;

  epsgbuf[0]='\0';
  srsbuffer[0]='\0';

  /* Some of the getMap parameters are actually required depending on the */
  /* request, but for now we assume all are optional and the map file */
  /* defaults will apply. */

  msAdjustExtent(&(map->extent), map->width, map->height);

  /*
    Check for SLDs first. If SLD is available LAYERS and STYLES parameters are non mandatory
   */
  for(i=0; map && i<numentries; i++) {
    /* check if SLD is passed.  If yes, check for OGR support */
    if (strcasecmp(names[i], "SLD") == 0 || strcasecmp(names[i], "SLD_BODY") == 0) {
      sldenabled = msOWSLookupMetadata(&(map->web.metadata), "MO", "sld_enabled");

      if (sldenabled == NULL) {
        sldenabled = "true";
      }

      if (strcasecmp(sldenabled, "true") == 0) {
        if (strcasecmp(names[i], "SLD") == 0) {
          sld_url =  values[i];
        }
        if (strcasecmp(names[i], "SLD_BODY") == 0) {
          sld_body =  values[i];
        }
      }
    }
  }

  for(i=0; map && i<numentries; i++) {
    /* getMap parameters */

    if (strcasecmp(names[i], "REQUEST") == 0) {
      request = values[i];
    }

    if (strcasecmp(names[i], "LAYERS") == 0) {
      int  j, k, iLayer, *layerOrder;
      int nLayerOrder = 0;
      char ***nestedGroups = NULL;
      int *numNestedGroups = NULL;
      int *isUsedInNestedGroup = NULL;

      layerOrder = (int*)malloc(map->numlayers * sizeof(int));
      MS_CHECK_ALLOC(layerOrder, map->numlayers * sizeof(int), MS_FAILURE)

      layers = msStringSplit(values[i], ',', &numwmslayerargs);
      if (layers==NULL || strlen(values[i]) <=0 ||   numwmslayerargs < 1) {
        numwmslayerargs = 0;
        if (sld_url == NULL &&   sld_body == NULL) {
          msFreeCharArray(layers,numwmslayerargs);
          msFree(layerOrder);
          msSetError(MS_WMSERR, "At least one layer name required in LAYERS.",
                     "msWMSLoadGetMapParams()");
          return msWMSException(map, nVersion, NULL, wms_exception_format);
        }
      }

      if (nVersion >= OWS_1_3_0) {
        layerlimit = msOWSLookupMetadata(&(map->web.metadata), "MO", "layerlimit");
        if(layerlimit) {
          if (numwmslayerargs > atoi(layerlimit)) {
            msFreeCharArray(layers,numwmslayerargs);
            msFree(layerOrder);
            msSetError(MS_WMSERR, "Number of layers requested exceeds LayerLimit.",
                       "msWMSLoadGetMapParams()");
            return msWMSException(map, nVersion, NULL, wms_exception_format);
          }
        }
      }

      for (iLayer=0; iLayer < map->numlayers; iLayer++) {
        map->layerorder[iLayer] = iLayer;
        layerOrder[iLayer] = 0;
      }

      for(j=0; j<map->numlayers; j++) {
        /* Keep only layers with status=DEFAULT by default */
        /* Layer with status DEFAULT is drawn first. */
        if (GET_LAYER(map, j)->status != MS_DEFAULT)
          GET_LAYER(map, j)->status = MS_OFF;
        else {
          map->layerorder[nLayerOrder++] = j;
          layerOrder[j] = 1;
        }
      }

      nestedGroups = (char***)msSmallCalloc(map->numlayers, sizeof(char**));
      numNestedGroups = (int*)msSmallCalloc(map->numlayers, sizeof(int));
      isUsedInNestedGroup = (int*)msSmallCalloc(map->numlayers, sizeof(int));
      msWMSPrepareNestedGroups(map, nVersion, nestedGroups, numNestedGroups, isUsedInNestedGroup);

      if (ows_request->layerwmsfilterindex != NULL)
        msFree(ows_request->layerwmsfilterindex);
      ows_request->layerwmsfilterindex = (int*)msSmallMalloc(map->numlayers * sizeof(int));
      for(j=0; j<map->numlayers; j++) {
        ows_request->layerwmsfilterindex[j] = -1;
      }
      ows_request->numwmslayerargs = numwmslayerargs;

      for (k=0; k<numwmslayerargs; k++) {
        layerfound = MS_FALSE;
        for (j=0; j<map->numlayers; j++) {
          /* Turn on selected layers only. */
          if ( ((GET_LAYER(map, j)->name &&
                 strcasecmp(GET_LAYER(map, j)->name, layers[k]) == 0) ||
                (map->name && strcasecmp(map->name, layers[k]) == 0) ||
                (GET_LAYER(map, j)->group && strcasecmp(GET_LAYER(map, j)->group, layers[k]) == 0) ||
                ((numNestedGroups[j] >0) && msStringInArray(layers[k], nestedGroups[j], numNestedGroups[j]))) &&
               ((msIntegerInArray(GET_LAYER(map, j)->index, ows_request->enabled_layers, ows_request->numlayers))) ) {
            if (GET_LAYER(map, j)->status != MS_DEFAULT) {
              if (layerOrder[j] == 0) {
                map->layerorder[nLayerOrder++] = j;
                layerOrder[j] = 1;
                GET_LAYER(map, j)->status = MS_ON;
              }
            }
            ows_request->layerwmsfilterindex[j] = k; /* Assign the corresponding filter */
            validlayers++;
            layerfound = MS_TRUE;
          }
        }
        if (layerfound == MS_FALSE && numwmslayerargs>0)
          invalidlayers++;

      }

      /* free the stuff used for nested layers */
      for (k = 0; k < map->numlayers; k++) {
        if (numNestedGroups[k] > 0) {
          msFreeCharArray(nestedGroups[k], numNestedGroups[k]);
        }
      }
      free(nestedGroups);
      free(numNestedGroups);
      free(isUsedInNestedGroup);

      /* set all layers with status off at end of array */
      for (j=0; j<map->numlayers; j++) {
        if (GET_LAYER(map, j)->status == MS_OFF)
          if (layerOrder[j] == 0)
            map->layerorder[nLayerOrder++] = j;
      }

      free(layerOrder);
      msFreeCharArray(layers, numwmslayerargs);
    } else if (strcasecmp(names[i], "STYLES") == 0) {
      styles = values[i];

    } else if ((strcasecmp(names[i], "SRS") == 0 && nVersion < OWS_1_3_0) ||
               (strcasecmp(names[i], "CRS") == 0 && nVersion >= OWS_1_3_0)) {
      srsfound = 1;
      /* SRS is in format "EPSG:epsg_id" or "AUTO:proj_id,unit_id,lon0,lat0" */
      if (strncasecmp(values[i], "EPSG:", 5) == 0) {
        /* SRS=EPSG:xxxx */

        /* don't need to copy init=xxx since the srsbudder is only
           used with msLoadProjection and that does alreay the job */
        /* snprintf(srsbuffer, 100, "init=epsg:%.20s", values[i]+5); */

        snprintf(srsbuffer, sizeof(srsbuffer), "EPSG:%.20s",values[i]+5);
        snprintf(epsgbuf, sizeof(epsgbuf), "EPSG:%.20s",values[i]+5);

        /* This test was to correct a request by the OCG cite 1.3.0 test
         sending CRS=ESPG:4326,  Bug:*/
        if (nVersion >= OWS_1_3_0) {
          if (srsbuffer[strlen(srsbuffer)-1] == ',')
            srsbuffer[strlen(srsbuffer)-1] = '\0';
          if (epsgbuf[strlen(epsgbuf)-1] == ',')
            epsgbuf[strlen(epsgbuf)-1] = '\0';
        }

        /* we need to wait until all params are read before */
        /* loding the projection into the map. This will help */
        /* insure that the passes srs is valid for all layers. */
        /*
        if (msLoadProjectionString(&(map->projection), buffer) != 0)
          return msWMSException(map, nVersion, NULL);

        iUnits = GetMapserverUnitUsingProj(&(map->projection));
        if (iUnits != -1)
          map->units = iUnits;
        */
      } else if (strncasecmp(values[i], "AUTO:", 5) == 0 && nVersion < OWS_1_3_0) {
        snprintf(srsbuffer, sizeof(srsbuffer), "%s",  values[i]);
        /* SRS=AUTO:proj_id,unit_id,lon0,lat0 */
        /*
        if (msLoadProjectionString(&(map->projection), values[i]) != 0)
          return msWMSException(map, nVersion, NULL);

        iUnits = GetMapserverUnitUsingProj(&(map->projection));
        if (iUnits != -1)
          map->units = iUnits;
        */
      } else if (nVersion >= OWS_1_3_0 && (strncasecmp(values[i], "AUTO2:", 6) == 0 ||
                                           strncasecmp(values[i], "CRS:", 4) == 0)) {
        snprintf(srsbuffer, sizeof(srsbuffer), "%s",  values[i]);
      } else {
        if (nVersion >= OWS_1_3_0) {
          msSetError(MS_WMSERR,
                     "Unsupported CRS namespace (only EPSG, AUTO2, CRS currently supported).",
                     "msWMSLoadGetMapParams()");
          return msWMSException(map, nVersion, "InvalidCRS", wms_exception_format);
        } else {
          msSetError(MS_WMSERR,
                     "Unsupported SRS namespace (only EPSG and AUTO currently supported).",
                     "msWMSLoadGetMapParams()");
          return msWMSException(map, nVersion, "InvalidSRS", wms_exception_format);
        }
      }
    } else if (strcasecmp(names[i], "BBOX") == 0) {
      char **tokens;
      int n;
      bboxfound = 1;
      tokens = msStringSplit(values[i], ',', &n);
      if (tokens==NULL || n != 4) {
        msSetError(MS_WMSERR, "Wrong number of arguments for BBOX.",
                   "msWMSLoadGetMapParams()");
        return msWMSException(map, nVersion, NULL, wms_exception_format);
      }
      map->extent.minx = atof(tokens[0]);
      map->extent.miny = atof(tokens[1]);
      map->extent.maxx = atof(tokens[2]);
      map->extent.maxy = atof(tokens[3]);

      msFreeCharArray(tokens, n);
      
      /*for wms 1.3.0 we will do the validation of the bbox after all parameters
       are read to account for the axes order*/
      if (nVersion < OWS_1_3_0) {

        /* validate bbox values */
        if ( map->extent.minx >= map->extent.maxx ||
             map->extent.miny >=  map->extent.maxy) {
          msSetError(MS_WMSERR,
                     "Invalid values for BBOX.",
                     "msWMSLoadGetMapParams()");
          return msWMSException(map, nVersion, NULL, wms_exception_format);
        }
        adjust_extent = MS_TRUE;
      }
    } else if (strcasecmp(names[i], "WIDTH") == 0) {
      widthfound = 1;
      map->width = atoi(values[i]);
    } else if (strcasecmp(names[i], "HEIGHT") == 0) {
      heightfound = 1;
      map->height = atoi(values[i]);
    } else if (strcasecmp(names[i], "FORMAT") == 0) {
      const char *format_list = NULL;
      formatfound = 1;

      if (strcasecmp(values[i], "application/openlayers")!=0) {
        /*check to see if a predefined list is given*/
        format_list = msOWSLookupMetadata(&(map->web.metadata), "M","getmap_formatlist");
        if (format_list) {
          format = msOwsIsOutputFormatValid(map, values[i], &(map->web.metadata),
                                            "M", "getmap_formatlist");
          if (format == NULL) {
            msSetError(MS_IMGERR,
                       "Unsupported output format (%s).",
                       "msWMSLoadGetMapParams()",
                       values[i] );
            return msWMSException(map, nVersion, "InvalidFormat", wms_exception_format);
          }
        } else {
          format = msSelectOutputFormat( map, values[i] );
          if( format == NULL ||
              (strncasecmp(format->driver, "MVT", 3) != 0 &&
               strncasecmp(format->driver, "GDAL/", 5) != 0 &&
               strncasecmp(format->driver, "AGG/", 4) != 0 &&
               strncasecmp(format->driver, "UTFGRID", 7) != 0 &&
               strncasecmp(format->driver, "CAIRO/", 6) != 0 &&
               strncasecmp(format->driver, "OGL/", 4) != 0 &&
               strncasecmp(format->driver, "KML", 3) != 0 &&
               strncasecmp(format->driver, "KMZ", 3) != 0)) {
            msSetError(MS_IMGERR,
                       "Unsupported output format (%s).",
                       "msWMSLoadGetMapParams()",
                       values[i] );
            return msWMSException(map, nVersion, "InvalidFormat", wms_exception_format);
          }
        }
      }
      msFree( map->imagetype );
      map->imagetype = msStrdup(values[i]);
    } else if (strcasecmp(names[i], "TRANSPARENT") == 0) {
      transparent = (strcasecmp(values[i], "TRUE") == 0);
    } else if (strcasecmp(names[i], "BGCOLOR") == 0) {
      long c;
      c = strtol(values[i], NULL, 16);
      map->imagecolor.red = (c/0x10000)&0xff;
      map->imagecolor.green = (c/0x100)&0xff;
      map->imagecolor.blue = c&0xff;
    }

    /* value of time can be empty. We should look for a default value */
    /* see function msWMSApplyTime */
    else if (strcasecmp(names[i], "TIME") == 0) { /* &&  values[i]) */
      stime = values[i];
      timerequest = 1;
    }
    /* Vendor-specific ANGLE param (for map rotation), added in ticket #3332,
     * also supported by GeoServer
     */
    else if (strcasecmp(names[i], "ANGLE") == 0) {
      msMapSetRotation(map, atof(values[i]));
    }
    /* Vendor-specific bbox_pixel_is_point, added in ticket #4652 */
    else if (strcasecmp(names[i], "BBOX_PIXEL_IS_POINT") == 0) {
      bbox_pixel_is_point = (strcasecmp(values[i], "TRUE") == 0);
    }
    /* Vendor-specific FILTER, added in RFC-118 */
    else if (strcasecmp(names[i], "FILTER") == 0) {
      filter = values[i];
    }
  }

  /*validate the exception format WMS 1.3.0 section 7.3.3.11*/

  if (nVersion >= OWS_1_3_0 && wms_exception_format != NULL) {
    if (strcasecmp(wms_exception_format, "INIMAGE") != 0 &&
        strcasecmp(wms_exception_format, "BLANK") != 0 &&
        strcasecmp(wms_exception_format, "XML") != 0) {
      msSetError(MS_WMSERR,
                 "Invalid format %s for the EXCEPTIONS parameter.",
                 "msWMSLoadGetMapParams()",  wms_exception_format);
      return msWMSException(map, nVersion, "InvalidFormat", wms_exception_format);
    }
  }
  if (bboxfound && nVersion >= OWS_1_3_0) {
    rectObj rect;
    projectionObj proj;

    /*we have already validated that the request format when reding
     the request parameters*/
    rect = map->extent;

    /*try to adjust the axes if necessary*/
    if (strlen(srsbuffer) > 1) {
      msInitProjection(&proj);
      msProjectionInheritContextFrom(&proj, &(map->projection));
      if (msLoadProjectionStringEPSG(&proj, (char *)srsbuffer) == 0 &&
	  (need_axis_swap = msIsAxisInvertedProj(&proj) ) ) {
        msAxisNormalizePoints( &proj, 1, &rect.minx, &rect.miny );
        msAxisNormalizePoints( &proj, 1, &rect.maxx, &rect.maxy );
      }
      msFreeProjection( &proj );
    }
    /*if the CRS is AUTO2:auto_crs_id,factor,lon0,lat0,
     we need to grab the factor patameter and use it with the bbox*/
    if (strlen(srsbuffer) > 1 && strncasecmp(srsbuffer, "AUTO2:", 6) == 0) {
      char **args;
      int numargs;
      double factor;
      args = msStringSplit(srsbuffer, ',', &numargs);
      if (numargs == 4) {
        factor = atof(args[1]);
        if (factor > 0 && factor != 1.0) {
          rect.minx = rect.minx*factor;
          rect.miny = rect.miny*factor;
          rect.maxx = rect.maxx*factor;
          rect.maxx = rect.maxy*factor;
        }
      }
      msFreeCharArray(args, numargs);
    }

    map->extent = rect;

    /* validate bbox values */
    if ( map->extent.minx >= map->extent.maxx ||
         map->extent.miny >=  map->extent.maxy) {
      msSetError(MS_WMSERR,
                 "Invalid values for BBOX.",
                 "msWMSLoadGetMapParams()");
      return msWMSException(map, nVersion, NULL, wms_exception_format);
    }
    adjust_extent = MS_TRUE;
  }

  /*
  ** If any select layers have a default time, we will apply the default
  ** time value even if no TIME request was in the url.
  */
  if( !timerequest && map ) {
    for (i=0; i<map->numlayers && !timerequest; i++) {
      layerObj *lp = NULL;

      lp = (GET_LAYER(map, i));
      if (lp->status != MS_ON && lp->status != MS_DEFAULT)
        continue;

      if( msOWSLookupMetadata(&(lp->metadata), "MO", "timedefault") )
        timerequest = 1;
    }
  }

  /*
  ** Apply time filters if available in the request.
  */
  if (timerequest) {
    if (msWMSApplyTime(map, nVersion, stime, wms_exception_format) == MS_FAILURE) {
      return MS_FAILURE;/* msWMSException(map, nVersion, "InvalidTimeRequest"); */
    }
  }

  /*
  ** Check/apply  wms dimensions
  ** all dimension requests shoul start with dim_xxxx, except time and elevation.
  */
  for (i=0; i<map->numlayers; i++) {
    layerObj *lp = NULL;
    const char *dimensionlist = NULL;
    char **tokens;
    int ntokens=0,k;

    lp = (GET_LAYER(map, i));
    if (lp->status != MS_ON && lp->status != MS_DEFAULT)
      continue;

    dimensionlist = msOWSLookupMetadata(&(lp->metadata), "M", "dimensionlist");
    /*
    if (!dimensionlist)
    dimensionlist = msOWSLookupMetadata(&(map->web.metadata), "M", "dimensionlist");
    */
    if (dimensionlist) {
      tokens = msStringSplit(dimensionlist,  ',', &ntokens);
      if (tokens && ntokens > 0) {
        char *dimensionname=NULL;
        char *stmp = NULL;
        for (j=0; j<ntokens; j++) {
          for(k=0; map && k<numentries; k++) {
            if (strcasecmp(names[k], "elevation") == 0)
              dimensionname = msStrdup(names[k]);
            else {

              /*
                dimensionname = msStrdup("dim_");
                dimensionname = msStringConcatenate(dimensionname, names[k]);
              */
              dimensionname = msStrdup(names[k]);
            }

            /*the dim_ is supposed to be part of the dimension name in the request*/
            if (strcasecmp(tokens[j], "elevation") == 0)
              stmp = msStrdup(tokens[j]);
            else {
              stmp =  msStrdup("dim_");
              msStringTrim(tokens[j]);
              stmp = msStringConcatenate(stmp, tokens[j]);
            }
            if (strcasecmp(dimensionname, stmp) == 0) {
              if (!msWMSApplyDimension(lp, nVersion, dimensionname, values[k],
                                       wms_exception_format)) {
                msFreeCharArray(tokens, ntokens);
                msFree(dimensionname);
                msFree(stmp);

                return msWMSException(lp->map, nVersion, "InvalidDimensionValue",  wms_exception_format);
              }
              msFree(dimensionname);
              msFree(stmp);
              break;
            }
            msFree(dimensionname);
            msFree(stmp);
          }
        }
        msFreeCharArray(tokens, ntokens);
      }
    }
  }

  /*
  ** Apply the selected output format (if one was selected), and override
  ** the transparency if needed.
  */

  if( format != NULL )
    msApplyOutputFormat( &(map->outputformat), format, transparent,
                         MS_NOOVERRIDE, MS_NOOVERRIDE );

  /* Validate all layers given.
  ** If an invalid layer is sent, return an exception.
  */
  if (validlayers == 0 || invalidlayers > 0) {
    if (invalidlayers > 0) {
      msSetError(MS_WMSERR, "Invalid layer(s) given in the LAYERS parameter. A layer might be disabled for \
this request. Check wms/ows_enable_request settings.",
                 "msWMSLoadGetMapParams()");
      return msWMSException(map, nVersion, "LayerNotDefined", wms_exception_format);
    }
    if (validlayers == 0 && sld_url == NULL &&   sld_body == NULL) {
      msSetError(MS_WMSERR, "Missing required parameter LAYERS", "msWMSLoadGetMapParams()");
      return msWMSException(map, nVersion, "MissingParameterValue", wms_exception_format);
    }
  }

  /* validate srs value: When the SRS parameter in a GetMap request contains a
  ** SRS that is valid for some, but not all of the layers being requested,
  ** then the server shall throw a Service Exception (code = "InvalidSRS").
  ** Validate first against epsg in the map and if no matching srs is found
  ** validate all layers requested.
  */
  if (epsgbuf[0] && epsgbuf[1]) { /*at least 2 chars*/
    char *projstring;
    epsgvalid = MS_FALSE;
    msOWSGetEPSGProj(&(map->projection), &(map->web.metadata),
                                  "MO", MS_FALSE, &projstring);
    if (projstring) {
      tokens = msStringSplit(projstring, ' ', &n);
      if (tokens && n > 0) {
        for(i=0; i<n; i++) {
          if (strcasecmp(tokens[i], epsgbuf) == 0) {
            epsgvalid = MS_TRUE;
            break;
          }
        }
        msFreeCharArray(tokens, n);
      }
      msFree(projstring);
    }
    if (epsgvalid == MS_FALSE) {
      for (i=0; i<map->numlayers; i++) {
        epsgvalid = MS_FALSE;
        if (GET_LAYER(map, i)->status == MS_ON) {
          msOWSGetEPSGProj(&(GET_LAYER(map, i)->projection),
                                        &(GET_LAYER(map, i)->metadata),
                                        "MO", MS_FALSE, &projstring);
          if (projstring) {
            tokens = msStringSplit(projstring, ' ', &n);
            if (tokens && n > 0) {
              for(j=0; j<n; j++) {
                if (strcasecmp(tokens[j], epsgbuf) == 0) {
                  epsgvalid = MS_TRUE;
                  break;
                }
              }
              msFreeCharArray(tokens, n);
            }
            msFree(projstring);
          }
          if (epsgvalid == MS_FALSE) {
            if (nVersion >= OWS_1_3_0) {
              msSetError(MS_WMSERR, "Invalid CRS given : CRS must be valid for all requested layers.",
                         "msWMSLoadGetMapParams()");
              return msWMSException(map, nVersion, "InvalidSRS", wms_exception_format);
            } else {
              msSetError(MS_WMSERR, "Invalid SRS given : SRS must be valid for all requested layers.",
                         "msWMSLoadGetMapParams()");
              return msWMSException(map, nVersion, "InvalidSRS", wms_exception_format);
            }
          }
        }
      }
    }
  }

  /* Validate requested image size.
   */
  if(map->width > map->maxsize || map->height > map->maxsize ||
      map->width < 1 || map->height < 1) {
    msSetError(MS_WMSERR, "Image size out of range, WIDTH and HEIGHT must be between 1 and %d pixels.", "msWMSLoadGetMapParams()", map->maxsize);

    /* Restore valid default values in case errors INIMAGE are used */
    map->width = 400;
    map->height= 300;
    return msWMSException(map, nVersion, NULL, wms_exception_format);
  }

  /* Check whether requested BBOX and width/height result in non-square pixels
   */
  nonsquare_enabled = msTestConfigOption( map, "MS_NONSQUARE", MS_FALSE );
  if (!nonsquare_enabled) {
    double dx, dy, reqy;
    dx = MS_ABS(map->extent.maxx - map->extent.minx);
    dy = MS_ABS(map->extent.maxy - map->extent.miny);

    reqy = ((double)map->width) * dy / dx;

    /* Allow up to 1 pixel of error on the width/height ratios. */
    /* If more than 1 pixel then enable non-square pixels */
    if ( MS_ABS((reqy - (double)map->height)) > 1.0 ) {
      if (map->debug)
        msDebug("msWMSLoadGetMapParams(): enabling non-square pixels.\n");
      msSetConfigOption(map, "MS_NONSQUARE", "YES");
      nonsquare_enabled = MS_TRUE;
    }
  }

  /* If the requested SRS is different from the default mapfile projection, or
  ** if a BBOX resulting in non-square pixels is requested then
  ** copy the original mapfile's projection to any layer that doesn't already
  ** have a projection. This will prevent problems when users forget to
  ** explicitly set a projection on all layers in a WMS mapfile.
  */
  if (strlen(srsbuffer) > 1 || nonsquare_enabled) {
    projectionObj newProj;

    if (map->projection.numargs <= 0) {
      if (nVersion >= OWS_1_3_0) {
        msSetError(MS_WMSERR, "Cannot set new CRS on a map that doesn't "
                   "have any projection set. Please make sure your mapfile "
                   "has a projection defined at the top level.",
                   "msWMSLoadGetMapParams()");
        return msWMSException(map, nVersion, "InvalidCRS", wms_exception_format);
      } else {
        msSetError(MS_WMSERR, "Cannot set new SRS on a map that doesn't "
                   "have any projection set. Please make sure your mapfile "
                   "has a projection defined at the top level.",
                   "msWMSLoadGetMapParams()");
        return msWMSException(map, nVersion, "InvalidSRS", wms_exception_format);
      }
    }

    msInitProjection(&newProj);
    msProjectionInheritContextFrom(&newProj, &map->projection);
    if (strlen(srsbuffer) > 1) {
      int nTmp;

      if (nVersion >= OWS_1_3_0)
        nTmp = msLoadProjectionStringEPSG(&newProj, srsbuffer);
      else
        nTmp = msLoadProjectionString(&newProj, srsbuffer);
      if (nTmp != 0) {
        msFreeProjection(&newProj);
        return msWMSException(map, nVersion, NULL, wms_exception_format);
      }
    }

    if (nonsquare_enabled ||
        msProjectionsDiffer(&(map->projection), &newProj)) {
      msMapSetLayerProjections(map);
    }
    msFreeProjection(&newProj);
  }

  /* apply the srs to the map file. This is only done after validating */
  /* that the srs given as parameter is valid for all layers */
  if (strlen(srsbuffer) > 1) {
    int nTmp;
    msFreeProjectionExceptContext(&map->projection);
    if (nVersion >= OWS_1_3_0)
      nTmp = msLoadProjectionStringEPSG(&(map->projection), srsbuffer);
    else
      nTmp = msLoadProjectionString(&(map->projection), srsbuffer);

    if (nTmp != 0)
      return msWMSException(map, nVersion, NULL, wms_exception_format);

    nTmp = GetMapserverUnitUsingProj(&(map->projection));
    if( nTmp != -1 ) {
      map->units = nTmp;
    }
  }

  if (sld_url || sld_body) {
    int nLayersBefore, nLayerAfter;
    char request_tmp[32];
    char *pszLayerNames = NULL;
    nLayersBefore = map->numlayers;

    /* -------------------------------------------------------------------- */
    /*      if LAYERS parameter was not given, set all layers to off        */
    /* -------------------------------------------------------------------- */
    if (validlayers == 0) { /*no LAYERS parameter is give*/
      for(j=0; j<map->numlayers; j++) {
        if (GET_LAYER(map, j)->status != MS_DEFAULT)
          GET_LAYER(map, j)->status = MS_OFF;
      }
    }

    /*apply sld if defined. This is done here so that bbox and srs are already applied*/
    if (sld_url) {
      if ((status = msSLDApplySLDURL(map, sld_url, -1, NULL, &pszLayerNames)) != MS_SUCCESS)
        return msWMSException(map, nVersion, NULL, wms_exception_format);
    } else if (sld_body) {
      if ((status =msSLDApplySLD(map, sld_body, -1, NULL, &pszLayerNames)) != MS_SUCCESS)
        return msWMSException(map, nVersion, NULL, wms_exception_format);
    }
    /* -------------------------------------------------------------------- */
    /*      SLD and styles can use the same layer multiple times. If        */
    /*      that is the case we duplicate the layer for drawing             */
    /*      purpose. We need to reset the ows request enable settings (#1602)*/
    /* -------------------------------------------------------------------- */
    nLayerAfter=map->numlayers;
    if (nLayersBefore != nLayerAfter) {
      strlcpy(request_tmp, "GetMap", sizeof(request_tmp));
      msOWSRequestLayersEnabled(map, "M", request_tmp, ows_request);
    }

    /* -------------------------------------------------------------------- */
    /*      We need to take into account where the LAYERS parameter was     */
    /*      not given (the LAYERS is option when an SLD is given). In       */
    /*      this particular case, we need to turn on the layers             */
    /*      identified the SLD (#1166).                                     */
    /*                                                                      */
    /* -------------------------------------------------------------------- */
    if (validlayers == 0) {
      if (pszLayerNames) {
        char **tokens;
        int ntokens=0;
        tokens = msStringSplit(pszLayerNames, ',', &ntokens);
        if (ntokens >0) {
          for (i=0; i<ntokens; i++) {
            for (j=0; j<map->numlayers; j++) {
              if ( ((GET_LAYER(map, j)->name &&
                     strcasecmp(GET_LAYER(map, j)->name, tokens[i]) == 0) ||
                    (map->name && strcasecmp(map->name, tokens[i]) == 0) ||
                    (GET_LAYER(map, j)->group && strcasecmp(GET_LAYER(map, j)->group, tokens[i]) == 0)) &&
                   ((msIntegerInArray(GET_LAYER(map, j)->index, ows_request->enabled_layers, ows_request->numlayers))) ) {
                if (GET_LAYER(map, j)->status != MS_DEFAULT)
                  GET_LAYER(map, j)->status = MS_ON;
              }
            }
          }
        }
      }
    }
    msFree(pszLayerNames);

  }

  /* Validate Styles :
  ** MapServer advertize styles through th group setting in a class object.
  ** If no styles are set MapServer expects to have empty values
  ** for the styles parameter (...&STYLES=&...) Or for multiple Styles/Layers,
  ** we could have ...&STYLES=,,,. If that is not the
  ** case, we generate an exception.
  */
  if(styles && strlen(styles) > 0) {
    char **tokens;
    int n=0, i=0, k=0, l=0,m=0;
    char **layers=NULL;
    int numlayers =0;
    layerObj *lp = NULL;

    tokens = msStringSplitComplex(styles, ",",&n,MS_ALLOWEMPTYTOKENS);
    for (i=0; i<n; i++) {
      if (tokens[i] && strlen(tokens[i]) > 0 &&
          strcasecmp(tokens[i],"default") != 0) {
        if (layers == NULL) {
          int bLayerInserted = MS_FALSE;
          char request_tmp[32];

          for(j=0; j<numentries; j++) {
            if (strcasecmp(names[j], "LAYERS") == 0) {
              layers = msStringSplit(values[j], ',', &numlayers);
            }
          }
          /* -------------------------------------------------------------------- */
          /*      If the same layer is given more that once, we need to           */
          /*      duplicate it.                                                   */
          /* -------------------------------------------------------------------- */
          if (layers && numlayers>0) {
            for (m=0; m<numlayers; m++) {
              layerObj *psTmpLayer=NULL;
              int nIndex;
              char tmpId[128];
              for (l=0; l<numlayers; l++) {
                nIndex = msGetLayerIndex(map, layers[m]);
                if (m !=l && strcasecmp(layers[m], layers[l])== 0 &&
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
                  psTmpLayer->name = msStrdup(tmpId);
                  msFree(layers[l]);
                  layers[l] = msStrdup(tmpId);
                  msInsertLayer(map, psTmpLayer, -1);
                  bLayerInserted =MS_TRUE;
                  /* layer was copied, we need to decrement its refcount */
                  MS_REFCNT_DECR(psTmpLayer);
                }
              }
            }
          }

          if (bLayerInserted) {
            strlcpy(request_tmp, "GetMap", sizeof(request_tmp));
            msOWSRequestLayersEnabled(map, "M", request_tmp, ows_request);
          }

        }
        if (layers && numlayers == n) {
          for (j=0; j<map->numlayers; j++) {
            if ((GET_LAYER(map, j)->name &&
                 strcasecmp(GET_LAYER(map, j)->name, layers[i]) == 0) ||
                (GET_LAYER(map, j)->group && strcasecmp(GET_LAYER(map, j)->group, layers[i]) == 0)) {
              lp =   GET_LAYER(map, j);
              for (k=0; k<lp->numclasses; k++) {
                if (lp->class[k]->group && strcasecmp(lp->class[k]->group, tokens[i]) == 0) {
                  msFree(lp->classgroup);
                  lp->classgroup = msStrdup( tokens[i]);
                  break;
                }
              }
              if (k == lp->numclasses) {
                msSetError(MS_WMSERR, "Style (%s) not defined on layer.",
                           "msWMSLoadGetMapParams()", tokens[i]);
                msFreeCharArray(tokens, n);
                msFreeCharArray(layers, numlayers);

                return msWMSException(map, nVersion, "StyleNotDefined", wms_exception_format);
              }
            /* Check the style of the root layer */
            } else if (map->name && strcasecmp(map->name, layers[i]) == 0) {
                const char *styleName = NULL;
                char *pszEncodedStyleName = NULL;
                styleName = msOWSLookupMetadata(&(map->web.metadata), "MO", "style_name");
                if (styleName == NULL)
                  styleName = "default";
                pszEncodedStyleName = msEncodeHTMLEntities(styleName);
                if (strcasecmp(pszEncodedStyleName, tokens[i]) != 0) {
                    msSetError(MS_WMSERR, "Style (%s) not defined on root layer.",
                               "msWMSLoadGetMapParams()", tokens[i]);
                    msFreeCharArray(tokens, n);
                    msFreeCharArray(layers, numlayers);
                    msFree(pszEncodedStyleName);

                    return msWMSException(map, nVersion, "StyleNotDefined", wms_exception_format);
                }
                msFree(pszEncodedStyleName);
            }

          }
        } else {
          msSetError(MS_WMSERR, "Invalid style (%s). Mapserver is expecting an empty string for the STYLES : STYLES= or STYLES=,,, or using keyword default  STYLES=default,default, ...",
                     "msWMSLoadGetMapParams()", styles);
          msFreeCharArray(tokens, n);
          msFreeCharArray(layers, numlayers);
          return msWMSException(map, nVersion, "StyleNotDefined", wms_exception_format);
        }
      }
    }
    msFreeCharArray(tokens, n);
    msFreeCharArray(layers, numlayers);
  }

  /*
  ** WMS extents are edge to edge while MapServer extents are center of
  ** pixel to center of pixel.  Here we try to adjust the WMS extents
  ** in by half a pixel.  We wait till here because we want to ensure we
  ** are doing this in terms of the correct WIDTH and HEIGHT.
  */
  if( adjust_extent && map->width>1 && map->height>1 && !bbox_pixel_is_point) {
    double  dx, dy;

    dx = (map->extent.maxx - map->extent.minx) / map->width;
    map->extent.minx += dx*0.5;
    map->extent.maxx -= dx*0.5;

    dy = (map->extent.maxy - map->extent.miny) / map->height;
    map->extent.miny += dy*0.5;
    map->extent.maxy -= dy*0.5;
  }

  if (request && strcasecmp(request, "DescribeLayer") != 0) {
    if (srsfound == 0) {
      if ( nVersion >= OWS_1_3_0)
        msSetError(MS_WMSERR, "Missing required parameter CRS", "msWMSLoadGetMapParams()");
      else
        msSetError(MS_WMSERR, "Missing required parameter SRS", "msWMSLoadGetMapParams()");

      return msWMSException(map, nVersion, "MissingParameterValue", wms_exception_format);
    }

    if (bboxfound == 0) {
      msSetError(MS_WMSERR, "Missing required parameter BBOX", "msWMSLoadGetMapParams()");
      return msWMSException(map, nVersion, "MissingParameterValue", wms_exception_format);
    }

    if (formatfound == 0 && (strcasecmp(request, "GetMap") == 0 || strcasecmp(request, "map") == 0)) {
      msSetError(MS_WMSERR, "Missing required parameter FORMAT", "msWMSLoadGetMapParams()");
      return msWMSException(map, nVersion, "MissingParameterValue", wms_exception_format);
    }

    if (widthfound == 0) {
      msSetError(MS_WMSERR, "Missing required parameter WIDTH", "msWMSLoadGetMapParams()");
      return msWMSException(map, nVersion, "MissingParameterValue", wms_exception_format);
    }

    if (heightfound == 0) {
      msSetError(MS_WMSERR, "Missing required parameter HEIGHT", "msWMSLoadGetMapParams()");
      return msWMSException(map, nVersion, "MissingParameterValue", wms_exception_format);
    }

  }

  /*
  ** Apply vendor-specific filter if specified
  */
  if (filter) {
    if (sld_url || sld_body) {
      msSetError(MS_WMSERR,
                 "Vendor-specific FILTER parameter cannot be used with SLD or SLD_BODY.",
                 "msWMSLoadGetMapParams()");
      return msWMSException(map, nVersion, NULL, wms_exception_format);
    }

    if (msWMSApplyFilter(map, nVersion, filter, need_axis_swap, wms_exception_format, ows_request) == MS_FAILURE) {
      return MS_FAILURE;/* msWMSException(map, nVersion, "InvalidFilterRequest"); */
    }
  }

  return MS_SUCCESS;
}

/*
**
*/
static void msWMSPrintRequestCap(int nVersion, const char *request,
                                 const char *script_url, const char *formats, ...)
{
  va_list argp;
  const char *fmt;
  char *encoded;

  msIO_printf("    <%s>\n", request);

  /* We expect to receive a NULL-terminated args list of formats */
  va_start(argp, formats);
  fmt = formats;
  while(fmt != NULL) {
    /* Special case for early WMS with subelements in Format (bug 908) */
    if( nVersion <= OWS_1_0_7 ) {
      encoded = msStrdup( fmt );
    }

    /* otherwise we HTML code special characters */
    else {
      encoded = msEncodeHTMLEntities(fmt);
    }

    msIO_printf("      <Format>%s</Format>\n", encoded);
    msFree(encoded);

    fmt = va_arg(argp, const char *);
  }
  va_end(argp);

  msIO_printf("      <DCPType>\n");
  msIO_printf("        <HTTP>\n");
  /* The URL should already be HTML encoded. */
  if (nVersion == OWS_1_0_0) {
    msIO_printf("          <Get onlineResource=\"%s\" />\n", script_url);
    msIO_printf("          <Post onlineResource=\"%s\" />\n", script_url);
  } else {
    msIO_printf("          <Get><OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:href=\"%s\"/></Get>\n", script_url);
    msIO_printf("          <Post><OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:href=\"%s\"/></Post>\n", script_url);
  }

  msIO_printf("        </HTTP>\n");
  msIO_printf("      </DCPType>\n");
  msIO_printf("    </%s>\n", request);
}



void msWMSPrintAttribution(FILE *stream, const char *tabspace,
                           hashTableObj *metadata,
                           const char *namespaces)
{
  const char *title, *onlineres, *logourl;
  char * pszEncodedValue=NULL;

  if (stream && metadata) {
    title = msOWSLookupMetadata(metadata, "MO",
                                "attribution_title");
    onlineres = msOWSLookupMetadata(metadata, "MO",
                                    "attribution_onlineresource");
    logourl = msOWSLookupMetadata(metadata, "MO",
                                  "attribution_logourl_width");

    if (title || onlineres || logourl) {
      msIO_printf("%s<Attribution>\n",tabspace);
      if (title) {
        pszEncodedValue = msEncodeHTMLEntities(title);
        msIO_fprintf(stream, "%s%s<Title>%s</Title>\n", tabspace,
                     tabspace, pszEncodedValue);
        free(pszEncodedValue);
      }

      if (onlineres) {
        pszEncodedValue = msEncodeHTMLEntities(onlineres);
        msIO_fprintf(stream, "%s%s<OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:href=\"%s\"/>\n", tabspace, tabspace,
                     pszEncodedValue);
        free(pszEncodedValue);
      }

      if (logourl) {
        msOWSPrintURLType(stream, metadata, "MO","attribution_logourl",
                          OWS_NOERR, NULL, "LogoURL", NULL,
                          " width=\"%s\"", " height=\"%s\"",
                          ">\n             <Format>%s</Format",
                          "\n             <OnlineResource "
                          "xmlns:xlink=\"http://www.w3.org/1999/xlink\""
                          " xlink:type=\"simple\" xlink:href=\"%s\"/>\n"
                          "          ",
                          MS_FALSE, MS_TRUE, MS_TRUE, MS_TRUE, MS_TRUE,
                          NULL, NULL, NULL, NULL, NULL, "        ");


      }
      msIO_printf("%s</Attribution>\n", tabspace);

    }
  }
}


/*
** msWMSPrintScaleHint()
**
** Print a Min/MaxScaleDenominator tag for the layer if applicable.
** used for WMS >=1.3.0
*/
void msWMSPrintScaleDenominator(const char *tabspace, double minscaledenom,
                                double maxscaledenom)
{
  if (minscaledenom > 0)
    msIO_printf("%s<MinScaleDenominator>%g</MinScaleDenominator>\n",
                tabspace, minscaledenom);

  if (maxscaledenom > 0)
    msIO_printf("%s<MaxScaleDenominator>%g</MaxScaleDenominator>\n",
                tabspace, maxscaledenom);
}



/*
** msWMSPrintScaleHint()
**
** Print a ScaleHint tag for this layer if applicable.
**
** (see WMS 1.1.0 sect. 7.1.5.4) The WMS defines the scalehint values as
** the ground distance in meters of the southwest to northeast diagonal of
** the central pixel of a map.  ScaleHint values are the min and max
** recommended values of that diagonal.
*/
void msWMSPrintScaleHint(const char *tabspace, double minscaledenom,
                         double maxscaledenom, double resolution)
{
  double scalehintmin=0.0, scalehintmax=0.0, diag;

  diag = sqrt(2.0);

  if (minscaledenom > 0)
    scalehintmin = diag*(minscaledenom/resolution)/msInchesPerUnit(MS_METERS,0);
  if (maxscaledenom > 0)
    scalehintmax = diag*(maxscaledenom/resolution)/msInchesPerUnit(MS_METERS,0);

  if (scalehintmin > 0.0 || scalehintmax > 0.0) {
    msIO_printf("%s<ScaleHint min=\"%.15g\" max=\"%.15g\" />\n",
                tabspace, scalehintmin, scalehintmax);
    if (scalehintmax == 0.0)
      msIO_printf("%s<!-- WARNING: Only MINSCALEDENOM and no MAXSCALEDENOM specified in "
                  "the mapfile. A default value of 0 has been returned for the "
                  "Max ScaleHint but this is probably not what you want. -->\n",
                  tabspace);
  }
}

/*
** msWMSPrintAuthorityURL()
**
** Print an AuthorityURL tag if applicable.
*/
void msWMSPrintAuthorityURL(FILE *stream, const char *tabspace,
                            hashTableObj *metadata, const char *namespaces)
{
  const char *pszWmsAuthorityName;
  const char *pszWMSAuthorityHref;
  char *pszTemplateName=NULL, *pszTemplateHref=NULL;

  if (stream && metadata) {
    pszWmsAuthorityName = msOWSLookupMetadata(metadata, namespaces, "authorityurl_name");
    pszWMSAuthorityHref = msOWSLookupMetadata(metadata, namespaces, "authorityurl_href");

    /* AuthorityURL only makes sense if you have *both* the name and url */
    if( pszWmsAuthorityName && pszWMSAuthorityHref ) {
      pszTemplateName = msStringConcatenate(msStrdup(tabspace), "<AuthorityURL name=\"%s\">\n");
      msOWSPrintEncodeMetadata(stream, metadata, namespaces, "authorityurl_name",
                               OWS_NOERR, pszTemplateName,
                               NULL);
      pszTemplateHref = msStringConcatenate(msStrdup(tabspace), "  <OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:href=\"%s\"/>\n");
      msOWSPrintEncodeMetadata(stream, metadata, namespaces, "authorityurl_href",
                               OWS_NOERR, pszTemplateHref,
                               NULL);
      msIO_printf("%s</AuthorityURL>\n", tabspace);
    } else if ( pszWmsAuthorityName || pszWMSAuthorityHref ) {
      msIO_printf("%s<!-- WARNING: Both wms_authorityurl_name and wms_authorityurl_href must be set to output an AuthorityURL -->\n", tabspace);
    }
  }
  msFree(pszTemplateName);
  msFree(pszTemplateHref);
}

/*
** msWMSPrintIdentifier()
**
** Print an Identifier tag if applicable.
*/
void msWMSPrintIdentifier(FILE *stream, const char *tabspace,
                          hashTableObj *metadata, const char *namespaces)
{
  const char *pszWMSIdentifierAuthority;
  const char *pszWMSIdentifierValue;
  char *pszTemplate=NULL;

  if (stream && metadata) {
    pszWMSIdentifierAuthority = msOWSLookupMetadata(metadata, namespaces, "identifier_authority");
    pszWMSIdentifierValue = msOWSLookupMetadata(metadata, namespaces, "identifier_value");

    /* Identifier only makes sense if you have *both* the authority and value */
    if( pszWMSIdentifierAuthority && pszWMSIdentifierValue ) {
      pszTemplate = msStringConcatenate(msStrdup(tabspace), "<Identifier authority=\"%s\">");
      msOWSPrintEncodeMetadata(stream, metadata, namespaces, "identifier_authority",
                               OWS_NOERR, pszTemplate,
                               NULL);
      msOWSPrintEncodeMetadata(stream, metadata, namespaces, "identifier_value",
                               OWS_NOERR, "%s</Identifier>\n",
                               NULL);
    } else if ( pszWMSIdentifierAuthority || pszWMSIdentifierValue ) {
      msIO_printf("%s<!-- WARNING: Both wms_identifier_authority and wms_identifier_value must be set to output an Identifier -->\n", tabspace);
    }
  }
  msFree(pszTemplate);
}

/*
** msWMSPrintKeywordlist()
**
** Print a Keywordlist tag if applicable.
*/
void msWMSPrintKeywordlist(FILE *stream, const char *tabspace,
                           const char *name, hashTableObj *metadata,
                           const char *namespaces, int nVersion)
{
  char newname[28];            /* max. rootlayer_keywordlist_items          */
  char vocname[33];            /* max. rootlayer_keywordlist_vocabulary     */
  const char *vocabularylist;  /* to be read from metadata                  */
  char **tokens = NULL;        /* splitted vocabularylist                   */
  char *pszName=NULL,          /* key to lookup keywords with vocabulary    */
        *pszTemplate1=NULL,     /* templates for msOWSPrintEncodeMetadataList*/
         *pszTemplate2=NULL;
  int i, sName, sTemplate, ntokens = 0;

  newname[0]='\0';
  vocname[0]='\0';
  snprintf(newname, sizeof(newname), "%s_items", name);
  snprintf(vocname, sizeof(vocname), "%s_vocabulary", name);

  if (nVersion == OWS_1_0_0) {
    /* <Keywords> in V 1.0.0 */
    /* The 1.0.0 spec doesn't specify which delimiter to use so let's use spaces */
    pszTemplate1 = msStringConcatenate(msStrdup(tabspace), "<Keywords>");
    pszTemplate2 = msStringConcatenate(msStrdup(tabspace), "</Keywords>\n");
    msOWSPrintEncodeMetadataList(stream, metadata, namespaces,
                                 name, pszTemplate1, pszTemplate2,
                                 "%s ", NULL);
  } else if ( msOWSLookupMetadata(metadata, namespaces, name) ||
              msOWSLookupMetadata(metadata, namespaces, newname) ||
              msOWSLookupMetadata(metadata, namespaces, vocname) ) {
    /* <KeywordList><Keyword> ... in V1.0.6+ */
    msIO_printf("%s<KeywordList>\n", tabspace);
    pszTemplate1 = msStringConcatenate(msStrdup(tabspace), "    <Keyword>%s</Keyword>\n");
    /* print old styled ..._keywordlist */
    msOWSPrintEncodeMetadataList(stream, metadata, namespaces,
                                 name, NULL, NULL,
                                 pszTemplate1, NULL);
    /* print new styled ..._keywordlist_items */
    msOWSPrintEncodeMetadataList(stream, metadata, namespaces,
                                 newname, NULL, NULL,
                                 pszTemplate1, NULL);

    /* find out if there's a vocabulary list set */
    vocabularylist = msOWSLookupMetadata(metadata, namespaces, vocname);
    if ( vocabularylist && nVersion >= OWS_1_3_0 ) {
      tokens = msStringSplit(vocabularylist, ',', &ntokens);
      if ( tokens && ntokens > 0 ) {
        /* In order to do malloc only once, the length of the metadata*/
        /* key to lookup is calculated from "rootlayer_keywordlist"   */
        /* plus "_items" plus the length of all vocabulary items      */
        /* concatenate.                                               */
        /* The same is done with template string used for printing the*/
        /* keyword list. Here the string is tabspace + "    <Keyword  */
        /* vocabulary="" + ... + "">%s</Keyword>\n"                   */
        sName = 22 + 6 + strlen(vocabularylist) +1;
        pszName = (char*)msSmallMalloc(sName);

        sTemplate = strlen(tabspace)+4+21+strlen(vocabularylist)+17+1;
        pszTemplate2 = (char*)msSmallMalloc(sTemplate);

        for (i=0; i<ntokens; i++) {
          snprintf(pszName, sName,
                   "%s_%s_items", name, tokens[i]);
          snprintf(pszTemplate2, sTemplate,
                   "%s    <Keyword vocabulary=\"%s\">%s</Keyword>\n",
                   tabspace, tokens[i], "%s");
          msOWSPrintEncodeMetadataList(stream, metadata, namespaces,
                                       pszName, NULL, NULL,
                                       pszTemplate2, NULL);
        }
        msFree(pszName);
      }
      msFreeCharArray(tokens, ntokens);
    }
    msIO_printf("%s</KeywordList>\n", tabspace);
  }
  msFree(pszTemplate1);
  msFree(pszTemplate2);
}

/*
** msDumpLayer()
*/
static
int msDumpLayer(mapObj *map, layerObj *lp, int nVersion,
                const char *script_url_encoded, const char *indent,
                const char *validated_language, int grouplayer,
                int hasQueryableSubLayers)
{
  rectObj ext;
  const char *value;
  const char *pszWmsTimeExtent, *pszWmsTimeDefault= NULL, *pszStyle=NULL;
  const char *pszLegendURL=NULL;
  char *pszMetadataName=NULL, *mimetype=NULL;
  char **classgroups = NULL;
  int iclassgroups=0;
  char szVersionBuf[OWS_VERSION_MAXLEN];
  size_t bufferSize = 0;
  const char *pszDimensionlist=NULL;
  char *pszMapEPSG,*pszLayerEPSG;

  /* if the layer status is set to MS_DEFAULT, output a warning */
  if (lp->status == MS_DEFAULT)
    msIO_fprintf(stdout, "<!-- WARNING: This layer has its status set to DEFAULT and will always be displayed when doing a GetMap request even if it is not requested by the client. This is not in line with the expected behavior of a WMS server. Using status ON or OFF is recommended. -->\n");

  if (nVersion <= OWS_1_0_7) {
    msIO_printf("%s    <Layer queryable=\"%d\">\n",
                indent, hasQueryableSubLayers || msIsLayerQueryable(lp));
  } else {
    /* 1.1.0 and later: opaque and cascaded are new. */
    int cascaded=0, opaque=0;
    if ((value = msOWSLookupMetadata(&(lp->metadata), "MO", "opaque")) != NULL)
      opaque = atoi(value);
    if (lp->connectiontype == MS_WMS)
      cascaded = 1;

    msIO_printf("%s    <Layer queryable=\"%d\" opaque=\"%d\" cascaded=\"%d\">\n",
                indent, hasQueryableSubLayers || msIsLayerQueryable(lp), opaque, cascaded);
  }

  if (lp->name && strlen(lp->name) > 0 &&
      (msIsXMLTagValid(lp->name) == MS_FALSE || isdigit(lp->name[0])))
    msIO_fprintf(stdout, "<!-- WARNING: The layer name '%s' might contain spaces or "
                 "invalid characters or may start with a number. This could lead to potential problems. -->\n",
                 lp->name);
  msOWSPrintEncodeParam(stdout, "LAYER.NAME", lp->name, OWS_NOERR,
                        "        <Name>%s</Name>\n", NULL);

  /* the majority of this section is dependent on appropriately named metadata in the LAYER object */
  msOWSPrintEncodeMetadata2(stdout, &(lp->metadata), "MO", "title",
                            OWS_WARN, "        <Title>%s</Title>\n", lp->name, validated_language);

  msOWSPrintEncodeMetadata2(stdout, &(lp->metadata), "MO", "abstract",
                            OWS_NOERR, "        <Abstract>%s</Abstract>\n", NULL, validated_language);

  msWMSPrintKeywordlist(stdout, "        ", "keywordlist", &(lp->metadata), "MO", nVersion);

  msOWSGetEPSGProj(&(map->projection),&(map->web.metadata), "MO", MS_FALSE, &pszMapEPSG);
  msOWSGetEPSGProj(&(lp->projection),&(lp->metadata), "MO", MS_FALSE, &pszLayerEPSG);
  if(pszMapEPSG == NULL) {
    /* If map has no proj then every layer MUST have one or produce a warning */
    if (nVersion > OWS_1_1_0) {
      /* starting 1.1.1 SRS are given in individual tags */
      if (nVersion >= OWS_1_3_0) {
        msOWSPrintEncodeParamList(stdout, "(at least one of) "
                                  "MAP.PROJECTION, LAYER.PROJECTION "
                                  "or wms_srs metadata",
                                  pszLayerEPSG,
                                  OWS_WARN, ' ', NULL, NULL,
                                  "        <CRS>%s</CRS>\n", NULL);
      }
      else {
        msOWSPrintEncodeParamList(stdout, "(at least one of) "
                                  "MAP.PROJECTION, LAYER.PROJECTION "
                                  "or wms_srs metadata",
                                  pszLayerEPSG,
                                  OWS_WARN, ' ', NULL, NULL,
                                  "        <SRS>%s</SRS>\n", NULL);
      }
    } else {
      msOWSPrintEncodeParam(stdout, "(at least one of) MAP.PROJECTION, "
                            "LAYER.PROJECTION or wms_srs metadata",
                            pszLayerEPSG,
                            OWS_WARN, "        <SRS>%s</SRS>\n", NULL);
    }
  } else {
    /* No warning required in this case since there's at least a map proj. */
    if (nVersion > OWS_1_1_0) {
      /* starting 1.1.1 SRS are given in individual tags */
      if (nVersion >=  OWS_1_3_0) {
        msOWSPrintEncodeParamList(stdout, "(at least one of) "
                                  "MAP.PROJECTION, LAYER.PROJECTION "
                                  "or wms_srs metadata",
                                  pszLayerEPSG,
                                  OWS_NOERR, ' ', NULL, NULL,
                                  "        <CRS>%s</CRS>\n", NULL);
      } else {
        msOWSPrintEncodeParamList(stdout, "(at least one of) "
                                  "MAP.PROJECTION, LAYER.PROJECTION "
                                  "or wms_srs metadata",
                                  pszLayerEPSG,
                                  OWS_NOERR, ' ', NULL, NULL,
                                  "        <SRS>%s</SRS>\n", NULL);
      }
    } else {
      msOWSPrintEncodeParam(stdout,
                            " LAYER.PROJECTION (or wms_srs metadata)",
                            pszLayerEPSG,
                            OWS_NOERR, "        <SRS>%s</SRS>\n", NULL);
    }
  }
  msFree(pszLayerEPSG);
  msFree(pszMapEPSG);

  /* If layer has no proj set then use map->proj for bounding box. */
  if (msOWSGetLayerExtent(map, lp, "MO", &ext) == MS_SUCCESS) {
    if(lp->projection.numargs > 0) {
      if (nVersion >= OWS_1_3_0)
        msOWSPrintEX_GeographicBoundingBox(stdout, "        ", &(ext),
                                           &(lp->projection));
      else
        msOWSPrintLatLonBoundingBox(stdout, "        ", &(ext),
                                    &(lp->projection), NULL, OWS_WMS);

      msOWSPrintBoundingBox( stdout,"        ", &(ext), &(lp->projection),
                             &(lp->metadata), &(map->web.metadata), "MO", nVersion );
    } else {
      if (nVersion >= OWS_1_3_0)
        msOWSPrintEX_GeographicBoundingBox(stdout, "        ", &(ext),
                                           &(map->projection));
      else
        msOWSPrintLatLonBoundingBox(stdout, "        ", &(ext),
                                    &(map->projection), NULL, OWS_WMS);
      msOWSPrintBoundingBox(stdout,"        ", &(ext), &(map->projection),
                            &(lp->metadata), &(map->web.metadata), "MO", nVersion );
    }
  } else {
    if (nVersion >= OWS_1_3_0)
      msIO_printf("        <!-- WARNING: Optional Ex_GeographicBoundingBox could not be established for this layer.  Consider setting the EXTENT in the LAYER object, or wms_extent metadata. Also check that your data exists in the DATA statement -->\n");
    else
      msIO_printf("        <!-- WARNING: Optional LatLonBoundingBox could not be established for this layer.  Consider setting the EXTENT in the LAYER object, or wms_extent metadata. Also check that your data exists in the DATA statement -->\n");
  }

  /* time support */
  pszWmsTimeExtent = msOWSLookupMetadata(&(lp->metadata), "MO", "timeextent");
  if (pszWmsTimeExtent) {
    pszWmsTimeDefault = msOWSLookupMetadata(&(lp->metadata),  "MO",
                                            "timedefault");

    if (nVersion >= OWS_1_3_0) {
      if (pszWmsTimeDefault)
        msIO_fprintf(stdout, "        <Dimension name=\"time\" units=\"ISO8601\" default=\"%s\" nearestValue=\"0\">%s</Dimension>\n",pszWmsTimeDefault, pszWmsTimeExtent);
      else
        msIO_fprintf(stdout, "        <Dimension name=\"time\" units=\"ISO8601\" nearestValue=\"0\">%s</Dimension>\n",pszWmsTimeExtent);
    }

    else {
      msIO_fprintf(stdout, "        <Dimension name=\"time\" units=\"ISO8601\"/>\n");
      if (pszWmsTimeDefault)
        msIO_fprintf(stdout, "        <Extent name=\"time\" default=\"%s\" nearestValue=\"0\">%s</Extent>\n",pszWmsTimeDefault, pszWmsTimeExtent);
      else
        msIO_fprintf(stdout, "        <Extent name=\"time\" nearestValue=\"0\">%s</Extent>\n",pszWmsTimeExtent);
    }
  }

  /*dimensions support: elevation + other user defined dimensions*/
  pszDimensionlist = msOWSLookupMetadata(&(lp->metadata), "M", "dimensionlist");
  if (pszDimensionlist) {
    char **tokens = NULL;
    int ntokens = 0;
    char *pszDimension=NULL, *pszDimensionItemName=NULL, *pszDimensionExtentName=NULL, *pszDimensionUnitName=NULL, *pszDimensionDefaultName=NULL;
    const char *pszDimensionItem=NULL, *pszDimensionExtent=NULL, *pszDimensionUnit=NULL, *pszDimensionDefault=NULL;
    int i;

    tokens = msStringSplit(pszDimensionlist,  ',', &ntokens);
    if (tokens && ntokens > 0) {
      for (i=0; i<ntokens; i++) {
        /*check if manadatory unit and extent are set. Item should also be set. default value is optional*/
        pszDimension = msStrdup(tokens[i]);
        msStringTrim(pszDimension);

        pszDimensionItemName = msStrdup(pszDimension);
        pszDimensionItemName = msStringConcatenate(pszDimensionItemName, "_item");
        pszDimensionItem = msOWSLookupMetadata(&(lp->metadata), "M", pszDimensionItemName);

        pszDimensionExtentName = msStrdup(pszDimension);
        pszDimensionExtentName = msStringConcatenate(pszDimensionExtentName, "_extent");
        pszDimensionExtent = msOWSLookupMetadata(&(lp->metadata), "M", pszDimensionExtentName);

        pszDimensionUnitName = msStrdup(pszDimension);
        pszDimensionUnitName = msStringConcatenate(pszDimensionUnitName, "_units");
        pszDimensionUnit = msOWSLookupMetadata(&(lp->metadata), "M", pszDimensionUnitName);

        pszDimensionDefaultName = msStrdup(pszDimension);
        pszDimensionDefaultName = msStringConcatenate(pszDimensionDefaultName, "_default");
        pszDimensionDefault = msOWSLookupMetadata(&(lp->metadata), "M", pszDimensionDefaultName);

        if (pszDimensionItem && pszDimensionExtent && pszDimensionUnit) {
          if (nVersion >= OWS_1_3_0) {
            if(pszDimensionDefault && strlen(pszDimensionDefault) > 0)
              msIO_fprintf(stdout, "        <Dimension name=\"%s\" units=\"%s\" default=\"%s\" multipleValues=\"1\" nearestValue=\"0\">%s</Dimension>\n",
                           pszDimension, pszDimensionUnit, pszDimensionDefault, pszDimensionExtent);
            else
              msIO_fprintf(stdout, "        <Dimension name=\"%s\" units=\"%s\"  multipleValues=\"1\"  nearestValue=\"0\">%s</Dimension>\n",
                           pszDimension, pszDimensionUnit, pszDimensionExtent);
          } else {
            msIO_fprintf(stdout, "        <Dimension name=\"%s\" units=\"%s\"/>\n", pszDimension, pszDimensionUnit);
            if(pszDimensionDefault && strlen(pszDimensionDefault) > 0)
              msIO_fprintf(stdout, "        <Extent name=\"%s\" default=\"%s\" nearestValue=\"0\">%s</Extent>\n",
                           pszDimension, pszDimensionDefault, pszDimensionExtent);
            else
              msIO_fprintf(stdout, "        <Extent name=\"%s\" nearestValue=\"0\">%s</Extent>\n",
                           pszDimension, pszDimensionExtent);
          }
        }
        msFree(pszDimension);
        msFree(pszDimensionItemName);
        msFree(pszDimensionUnitName);
        msFree(pszDimensionExtentName);
        msFree(pszDimensionDefaultName);

      }
    }
    msFreeCharArray(tokens, ntokens);
  }


  if (nVersion >= OWS_1_0_7) {
    msWMSPrintAttribution(stdout, "    ", &(lp->metadata), "MO");
  }

  /* AuthorityURL support and Identifier support, only available >= WMS 1.1.0 */
  if(nVersion >= OWS_1_1_0) {
    msWMSPrintAuthorityURL(stdout, "        ", &(lp->metadata), "MO");
    msWMSPrintIdentifier(stdout, "        ", &(lp->metadata), "MO");
  }

  if(nVersion >= OWS_1_1_0) {
    if (! msOWSLookupMetadata(&(lp->metadata), "MO", "metadataurl_href"))
      msMetadataSetGetMetadataURL(lp, script_url_encoded);

    msOWSPrintURLType(stdout, &(lp->metadata), "MO", "metadataurl",
                      OWS_NOERR, NULL, "MetadataURL", " type=\"%s\"",
                      NULL, NULL, ">\n          <Format>%s</Format",
                      "\n          <OnlineResource xmlns:xlink=\""
                      "http://www.w3.org/1999/xlink\" "
                      "xlink:type=\"simple\" xlink:href=\"%s\"/>\n        ",
                      MS_TRUE, MS_FALSE, MS_FALSE, MS_TRUE, MS_TRUE,
                      NULL, NULL, NULL, NULL, NULL, "        ");
  }

  if(nVersion < OWS_1_1_0)
    msOWSPrintEncodeMetadata(stdout, &(lp->metadata), "MO", "dataurl_href",
                             OWS_NOERR, "        <DataURL>%s</DataURL>\n",
                             NULL);
  else
    msOWSPrintURLType(stdout, &(lp->metadata), "MO", "dataurl",
                      OWS_NOERR, NULL, "DataURL", NULL, NULL, NULL,
                      ">\n          <Format>%s</Format",
                      "\n          <OnlineResource xmlns:xlink=\""
                      "http://www.w3.org/1999/xlink\" "
                      "xlink:type=\"simple\" xlink:href=\"%s\"/>\n        ",
                      MS_FALSE, MS_FALSE, MS_FALSE, MS_TRUE, MS_TRUE,
                      NULL, NULL, NULL, NULL, NULL, "        ");

  /* The LegendURL reside in a style. The Web Map Context spec already  */
  /* included the support on this in mapserver. However, it is not in the  */
  /* wms_legendurl_... metadatas it's in the styles metadata, */
  /* In wms_style_<style_name>_lengendurl_... metadata. So we have to detect */
  /* the current style before reading it. Also in the Style block, we need */
  /* a Title and a name. Title is derived from wms_style_<style>_title, */
  /* which allows mutiple style definitions, e.g. by using classgroups. */
  bufferSize = 512; /* We don't know about possible style name lengths, yet */
  pszMetadataName = (char*)msSmallMalloc(bufferSize);
  pszStyle = msOWSLookupMetadata(&(lp->metadata), "MO", "style");
  if (pszStyle) {
    snprintf(pszMetadataName, bufferSize, "style_%s_legendurl_href", pszStyle);
    pszLegendURL = msOWSLookupMetadata(&(lp->metadata), "MO", pszMetadataName);
  } else
    pszStyle = "default";


  if(nVersion <= OWS_1_0_0 && pszLegendURL) {
    /* First, print the style block */
    msIO_fprintf(stdout, "        <Style>\n");
    msIO_fprintf(stdout, "          <Name>%s</Name>\n", pszStyle);
    /* Print the real Title or Style name otherwise */
    snprintf(pszMetadataName, bufferSize, "style_%s_title", pszStyle);
    msOWSPrintEncodeMetadata2(stdout, &(lp->metadata), "MO",
                              pszMetadataName,
                              OWS_NOERR,
                              "          <Title>%s</Title>\n",
                              pszStyle, validated_language);

    /* Inside, print the legend url block */
    snprintf(pszMetadataName, bufferSize, "style_%s_legendurl_href", pszStyle);
    msOWSPrintEncodeMetadata(stdout, &(lp->metadata), "MO",
                             pszMetadataName,
                             OWS_NOERR,
                             "          <StyleURL>%s</StyleURL>\n",
                             NULL);

    /* close the style block */
    msIO_fprintf(stdout, "        </Style>\n");

  } else if(nVersion >= OWS_1_1_0) {
    if (pszLegendURL) {
      /* First, print the style block */
      msIO_fprintf(stdout, "        <Style>\n");
      msIO_fprintf(stdout, "          <Name>%s</Name>\n", pszStyle);
      /* Print the real Title or Style name otherwise */
      snprintf(pszMetadataName, bufferSize, "style_%s_title", pszStyle);
      msOWSPrintEncodeMetadata2(stdout, &(lp->metadata), "MO",
                                pszMetadataName,
                                OWS_NOERR,
                                "          <Title>%s</Title>\n",
                                pszStyle, validated_language);


      /* Inside, print the legend url block */
      snprintf(pszMetadataName, bufferSize, "style_%s_legendurl", pszStyle);
      msOWSPrintURLType(stdout, &(lp->metadata), "MO",pszMetadataName,
                        OWS_NOERR, NULL, "LegendURL", NULL,
                        " width=\"%s\"", " height=\"%s\"",
                        ">\n             <Format>%s</Format",
                        "\n             <OnlineResource "
                        "xmlns:xlink=\"http://www.w3.org/1999/xlink\""
                        " xlink:type=\"simple\" xlink:href=\"%s\"/>\n"
                        "          ",
                        MS_FALSE, MS_TRUE, MS_TRUE, MS_TRUE, MS_TRUE,
                        NULL, NULL, NULL, NULL, NULL, "          ");
      msIO_fprintf(stdout, "        </Style>\n");

    } else {
      if (script_url_encoded) {
        if (lp->connectiontype != MS_WMS &&
            lp->connectiontype != MS_WFS &&
            lp->connectiontype != MS_UNUSED_1 &&
            lp->numclasses > 0) {
          char width[10], height[10];
          char *legendurl = NULL;
          size_t bufferSize = 0;
          int classnameset = 0, i=0;
          for (i=0; i<lp->numclasses; i++) {
            if (lp->class[i]->name &&
                strlen(lp->class[i]->name) > 0) {
              classnameset = 1;
              break;
            }
          }
          if (classnameset) {
            int j=0, k=0, l=0, size_x=0, size_y=0, num_layers=0;
            int *group_layers = (int *)msSmallMalloc(sizeof(int)*map->numlayers);
            char ***nestedGroups = NULL;
            int *numNestedGroups = NULL;
            int *isUsedInNestedGroup = NULL;

            nestedGroups = (char***)msSmallCalloc(map->numlayers, sizeof(char**));
            numNestedGroups = (int*)msSmallCalloc(map->numlayers, sizeof(int));
            isUsedInNestedGroup = (int*)msSmallCalloc(map->numlayers, sizeof(int));
            msWMSPrepareNestedGroups(map, nVersion, nestedGroups, numNestedGroups, isUsedInNestedGroup);

            num_layers = 1;
            group_layers[0] = lp->index;
            if (isUsedInNestedGroup[lp->index]) {
              for (j=0; j < map->numlayers; j++) {
                if (j == lp->index)
                    continue;
                for(k = 0; k < numNestedGroups[j]; k++) {
                  if (strcasecmp(lp->name, nestedGroups[j][k]) == 0) {
                    group_layers[num_layers++] = j;
                    break;
                  }
                }
              }
            }
            group_layers =(int *)msSmallRealloc(group_layers, sizeof(int)*num_layers);
            
            if (msLegendCalcSize(map, 1, &size_x, &size_y,  group_layers, num_layers, NULL, 1) == MS_SUCCESS) {
              const char *styleName = NULL;
              char *pszEncodedStyleName = NULL;
              layerObj *lp2 = NULL;

              snprintf(width, sizeof(width), "%d", size_x);
              snprintf(height, sizeof(height), "%d", size_y);

              bufferSize = strlen(script_url_encoded)+300;
              legendurl = (char*)msSmallMalloc(bufferSize);

#if defined USE_PNG
              mimetype = msEncodeHTMLEntities("image/png");
#endif

#if defined USE_JPEG
              if (!mimetype)
                mimetype = msEncodeHTMLEntities("image/jpeg");
#endif
              if (!mimetype)
                mimetype = msEncodeHTMLEntities(MS_IMAGE_MIME_TYPE(map->outputformat));

              /* -------------------------------------------------------------------- */
              /*      check if the group parameters for the classes are set. We       */
              /*      should then publish the different class groups as different styles.*/
              /* -------------------------------------------------------------------- */
              iclassgroups = 0;
              classgroups = NULL;

              styleName = msOWSLookupMetadata(&(map->web.metadata), "MO", "style_name");
              if (styleName == NULL)
                styleName = "default";
              pszEncodedStyleName = msEncodeHTMLEntities(styleName);

              for (i=0; i<lp->numclasses; i++) {
                if (lp->class[i]->name && lp->class[i]->group) {
                  /* Check that style is not inherited from root layer (#4442). */
                  if (strcasecmp(pszEncodedStyleName, lp->class[i]->group) == 0)
                    continue;
                  /* Check that style is not inherited from group layer(s) (#4442). */
                  if (numNestedGroups[lp->index] > 0) {
                    for (j=0; j<numNestedGroups[lp->index]; j++) {
                      for (k=0; k < map->numlayers; k++) {
                        if (GET_LAYER(map, k)->name && strcasecmp(GET_LAYER(map, k)->name, nestedGroups[lp->index][j]) == 0) {
                          lp2 = (GET_LAYER(map, k));
                          for (l=0; l < lp2->numclasses; l++) {
                            if (strcasecmp(lp2->class[l]->group, lp->class[i]->group) == 0)
                              break;
                          }
                          break;
                        }
                      }
                      if (lp2 && l < lp2->numclasses)
                        break;
                    }
                    if (j < numNestedGroups[lp->index])
                      continue;
                  }
                  if (!classgroups) {
                    classgroups = (char **)msSmallMalloc(sizeof(char *));
                    classgroups[iclassgroups++]= msStrdup(lp->class[i]->group);
                  } else {
                    /* Output style only once. */
                    for (j=0; j<iclassgroups; j++) {
                      if (strcasecmp(classgroups[j], lp->class[i]->group) == 0)
                        break;
                    }
                    if (j == iclassgroups) {
                      iclassgroups++;
                      classgroups = (char **)msSmallRealloc(classgroups, sizeof(char *)*iclassgroups);
                      classgroups[iclassgroups-1]= msStrdup(lp->class[i]->group);
                    }
                  }
                }
              }
              msFree(pszEncodedStyleName);
              if (classgroups == NULL) {
                classgroups = (char **)msSmallMalloc(sizeof(char *));
                classgroups[0]= msStrdup("default");
                iclassgroups = 1;
              }
              for (i=0; i<iclassgroups; i++) {
                char *name_encoded = msEncodeHTMLEntities(lp->name);
                char *classgroup_encoded = msEncodeHTMLEntities(classgroups[i]);
                if (nVersion >= OWS_1_3_0)
                  snprintf(legendurl, bufferSize, "%sversion=%s&amp;service=WMS&amp;request=GetLegendGraphic&amp;sld_version=1.1.0&amp;layer=%s&amp;format=%s&amp;STYLE=%s",
                           script_url_encoded,msOWSGetVersionString(nVersion, szVersionBuf),name_encoded,
                           mimetype,  classgroup_encoded);
                else
                  snprintf(legendurl, bufferSize, "%sversion=%s&amp;service=WMS&amp;request=GetLegendGraphic&amp;layer=%s&amp;format=%s&amp;STYLE=%s",
                           script_url_encoded,msOWSGetVersionString(nVersion, szVersionBuf),name_encoded,
                           mimetype,  classgroup_encoded);

                msFree(name_encoded);
                msFree(classgroup_encoded);

                msIO_fprintf(stdout, "        <Style>\n");
                msIO_fprintf(stdout, "          <Name>%s</Name>\n",  classgroups[i]);
                snprintf(pszMetadataName, bufferSize, "style_%s_title", classgroups[i]);
                msOWSPrintEncodeMetadata2(stdout, &(lp->metadata), "MO",
                                          pszMetadataName,
                                          OWS_NOERR,
                                          "          <Title>%s</Title>\n",
                                          classgroups[i], validated_language);

                /* A legendurl from wms_style_<style>_legendurl_href will override a self generated legend graphic */
                snprintf(pszMetadataName, bufferSize, "style_%s_legendurl_href", classgroups[i]);
                pszLegendURL = msOWSLookupMetadata(&(lp->metadata), "MO", pszMetadataName);
                if (pszLegendURL) {
                  snprintf(pszMetadataName, bufferSize, "style_%s_legendurl", classgroups[i]);
                  msOWSPrintURLType(stdout, &(lp->metadata),
                                    "MO", pszMetadataName,
                                    OWS_NOERR, NULL,
                                    "LegendURL", NULL,
                                    " width=\"%s\"", " height=\"%s\"",
                                    ">\n             <Format>%s</Format",
                                    "\n             <OnlineResource "
                                    "xmlns:xlink=\"http://www.w3.org/1999/xlink\""
                                    " xlink:type=\"simple\" xlink:href=\"%s\"/>\n"
                                    "          ",
                                    MS_FALSE, MS_TRUE, MS_TRUE, MS_TRUE, MS_TRUE,
                                    NULL, NULL, NULL, NULL, NULL, "          ");
                } else {
                  msOWSPrintURLType(stdout, NULL,
                                    "O", "ttt",
                                    OWS_NOERR, NULL,
                                    "LegendURL", NULL,
                                    " width=\"%s\"", " height=\"%s\"",
                                    ">\n             <Format>%s</Format",
                                    "\n             <OnlineResource "
                                    "xmlns:xlink=\"http://www.w3.org/1999/xlink\""
                                    " xlink:type=\"simple\" xlink:href=\"%s\"/>\n"
                                    "          ",
                                    MS_FALSE, MS_FALSE, MS_FALSE, MS_FALSE, MS_FALSE,
                                    NULL, width, height, mimetype, legendurl, "          ");
                }
                msIO_fprintf(stdout, "        </Style>\n");
              }
              msFree(legendurl);
              msFreeCharArray(classgroups, iclassgroups);
              msFree(mimetype);
            }
            /* free the stuff used for nested layers */
            for (i = 0; i < map->numlayers; i++) {
              if (numNestedGroups[i] > 0) {
                msFreeCharArray(nestedGroups[i], numNestedGroups[i]);
              }
            }
            free(nestedGroups);
            free(numNestedGroups);
            free(isUsedInNestedGroup);
            free(group_layers);
          }
        }
      }
    }
  }

  msFree(pszMetadataName);

  /* print Min/Max ScaleDenominator */
  if (nVersion <  OWS_1_3_0)
    msWMSPrintScaleHint("        ", lp->minscaledenom, lp->maxscaledenom, map->resolution);
  else
    msWMSPrintScaleDenominator("        ", lp->minscaledenom, lp->maxscaledenom);

  if ( grouplayer == MS_FALSE )
    msIO_printf("%s    </Layer>\n", indent);

  return MS_SUCCESS;
}

/*
 * msWMSIsSubGroup
 */
int msWMSIsSubGroup(char** currentGroups, int currentLevel, char** otherGroups, int numOtherGroups)
{
  int i;
  /* no match if otherGroups[] has less levels than currentLevel */
  if (numOtherGroups <= currentLevel) {
    return MS_FALSE;
  }
  /* compare all groups below the current level */
  for (i = 0; i <= currentLevel; i++) {
    if (strcmp(currentGroups[i], otherGroups[i]) != 0) {
      return MS_FALSE; /* if one of these is not equal it is not a sub group */
    }
  }
  return MS_TRUE;
}

/*
 * msWMSHasQueryableSubLayers
 */
static int msWMSHasQueryableSubLayers(mapObj* map, int index, int level,
                                      char*** nestedGroups, int* numNestedGroups)
{
    int j;
    for (j = index; j < map->numlayers; j++) {
      if (msWMSIsSubGroup(nestedGroups[index], level, nestedGroups[j], numNestedGroups[j])) {
          if( msIsLayerQueryable( GET_LAYER(map,j) ) )
              return MS_TRUE;
      }
    }
    return MS_FALSE;
}

/***********************************************************************************
 * msWMSPrintNestedGroups()                                                        *
 *                                                                                 *
 * purpose: Writes the layers to the capabilities that have the                    *
 * "WMS_LAYER_GROUP" metadata set.                                                 *
 *                                                                                 *
 * params:                                                                         *
 * -map: The main map object                                                       *
 * -nVersion: OGC WMS version                                                      *
 * -pabLayerProcessed: boolean array indicating which layers have been dealt with. *
 * -index: the index of the current layer.                                         *
 * -level: the level of depth in the group tree (root = 0)                         *
 * -nestedGroups: This array holds the arrays of groups that have                  *
 *   been set through the WMS_LAYER_GROUP metadata                                 *
 * -numNestedGroups: This array holds the number of nested groups for each layer   *
 ***********************************************************************************/
void msWMSPrintNestedGroups(mapObj* map, int nVersion, char* pabLayerProcessed,
                            int index, int level, char*** nestedGroups, int* numNestedGroups, int* isUsedInNestedGroup, const char *script_url_encoded, const char *validated_language)
{
  int i, j;
  int groupAdded = 0;
  char *indent = NULL;
  indent = msStrdup("");

  for (i = 0; i < level; i++) {
    indent = msStringConcatenate(indent, "  ");
  }

  if (numNestedGroups[index] <= level) { /* no more subgroups */
    if ((!pabLayerProcessed[index]) && (!isUsedInNestedGroup[index])) {
      /* we are at the deepest level of the group branchings, so add layer now. */
      msDumpLayer(map, GET_LAYER(map, index), nVersion, script_url_encoded, indent, validated_language, MS_FALSE, MS_FALSE);
      pabLayerProcessed[index] = 1; /* done */
    }
  } else { /* not yet there, we have to deal with this group and possible subgroups and layers. */
    for (j = 0; j < map->numlayers; j++) {
      if ( GET_LAYER(map, j)->name && strcasecmp(GET_LAYER(map, j)->name, nestedGroups[index][level]) == 0 ) {
        break;
      }
    }

    /* Beginning of a new group... enclose the group in a layer block */
    if ( j < map->numlayers ) {
      if (!pabLayerProcessed[j]) {
        msDumpLayer(map, GET_LAYER(map, j), nVersion, script_url_encoded,
                    indent, validated_language, MS_TRUE,
                    msWMSHasQueryableSubLayers(map, index, level,
                            nestedGroups, numNestedGroups));
        pabLayerProcessed[j] = 1; /* done */
        groupAdded = 1;
      }
    } else {
      msIO_printf("%s    <Layer%s>\n", indent,
        msWMSHasQueryableSubLayers(map, index, level,
                    nestedGroups, numNestedGroups) ? " queryable=\"1\"" : "");
      msIO_printf("%s      <Name>%s</Name>\n", indent, nestedGroups[index][level]);
      msIO_printf("%s      <Title>%s</Title>\n", indent, nestedGroups[index][level]);
      groupAdded = 1;
    }

    /* Look for one group deeper in the current layer */
    if (!pabLayerProcessed[index]) {
      msWMSPrintNestedGroups(map, nVersion, pabLayerProcessed,
                             index, level + 1, nestedGroups,
                             numNestedGroups, isUsedInNestedGroup,
                             script_url_encoded, validated_language);
    }

    /* look for subgroups in other layers. */
    for (j = index + 1; j < map->numlayers; j++) {
      if (msWMSIsSubGroup(nestedGroups[index], level, nestedGroups[j], numNestedGroups[j])) {
        if (!pabLayerProcessed[j]) {
          msWMSPrintNestedGroups(map, nVersion, pabLayerProcessed,
                                 j, level + 1, nestedGroups,
                                 numNestedGroups, isUsedInNestedGroup,
                                 script_url_encoded, validated_language);
        }
      } else {
        /* TODO: if we would sort all layers on "WMS_LAYER_GROUP" beforehand */
        /* we could break out of this loop at this point, which would increase */
        /* performance.  */
      }
    }
    /* Close group layer block */
    if (groupAdded)
      msIO_printf("%s    </Layer>\n", indent);
  }

  msFree(indent);
} /* msWMSPrintNestedGroups */

/*
** msWMSGetCapabilities()
*/
int msWMSGetCapabilities(mapObj *map, int nVersion, cgiRequestObj *req, owsRequestObj *ows_request,
                         const char *requested_updatesequence, char *wms_exception_format, const char *requested_language)
{
  char *dtd_url = NULL;
  char *script_url=NULL, *script_url_encoded=NULL;

  char szVersionBuf[OWS_VERSION_MAXLEN];
  char *schemalocation = NULL;
  const char *updatesequence=NULL;
  const char *sldenabled=NULL;
  const char *layerlimit=NULL;
  const char *rootlayer_name=NULL;
  char *pszTmp=NULL;
  int i;
  const char *format_list=NULL;
  char **tokens = NULL;
  int numtokens = 0;
  char *validated_language = NULL;
  char *pszMapEPSG;

  updatesequence = msOWSLookupMetadata(&(map->web.metadata), "MO", "updatesequence");

  sldenabled = msOWSLookupMetadata(&(map->web.metadata), "MO", "sld_enabled");

  if (sldenabled == NULL)
    sldenabled = "true";

  if (requested_updatesequence != NULL) {
    i = msOWSNegotiateUpdateSequence(requested_updatesequence, updatesequence);
    if (i == 0) { /* current */
      msSetError(MS_WMSERR, "UPDATESEQUENCE parameter (%s) is equal to server (%s)", "msWMSGetCapabilities()", requested_updatesequence, updatesequence);
      return msWMSException(map, nVersion, "CurrentUpdateSequence", wms_exception_format);
    }
    if (i > 0) { /* invalid */
      msSetError(MS_WMSERR, "UPDATESEQUENCE parameter (%s) is higher than server (%s)", "msWMSGetCapabilities()", requested_updatesequence, updatesequence);
      return msWMSException(map, nVersion, "InvalidUpdateSequence", wms_exception_format);
    }
  }

  schemalocation = msEncodeHTMLEntities( msOWSGetSchemasLocation(map) );

  if (nVersion < 0)
    nVersion = OWS_1_3_0;     /* Default to 1.3.0 */

  /* Decide which version we're going to return. */
  if (nVersion < OWS_1_0_7) {
    nVersion = OWS_1_0_0;
    dtd_url = msStrdup(schemalocation);
    dtd_url = msStringConcatenate(dtd_url, "/wms/1.0.0/capabilities_1_0_0.dtd");
  }

  else if (nVersion < OWS_1_1_0) {
    nVersion = OWS_1_0_7;
    dtd_url = msStrdup(schemalocation);
    dtd_url = msStringConcatenate(dtd_url, "/wms/1.0.7/capabilities_1_0_7.dtd");
  } else if (nVersion < OWS_1_1_1) {
    nVersion = OWS_1_1_0;
    dtd_url = msStrdup(schemalocation);
    dtd_url = msStringConcatenate(dtd_url, "/wms/1.1.0/capabilities_1_1_0.dtd");
  } else if (nVersion < OWS_1_3_0) {
    nVersion = OWS_1_1_1;
    dtd_url = msStrdup(schemalocation);
    /* this exception was added to accomadote the OGC test suite (Bug 1576)*/
    if (strcasecmp(schemalocation, OWS_DEFAULT_SCHEMAS_LOCATION) == 0)
      dtd_url = msStringConcatenate(dtd_url, "/wms/1.1.1/WMS_MS_Capabilities.dtd");
    else
      dtd_url = msStringConcatenate(dtd_url, "/wms/1.1.1/capabilities_1_1_1.dtd");
  } else
    nVersion = OWS_1_3_0;

  /* This function owns validated_language, so remember to free it later*/
  validated_language = msOWSGetLanguageFromList(map, "MO", requested_language);

  if (validated_language != NULL) {
    msMapSetLanguageSpecificConnection(map, validated_language);
  }

  /* We need this server's onlineresource. */
  /* Default to use the value of the "onlineresource" metadata, and if not */
  /* set then build it: "http://$(SERVER_NAME):$(SERVER_PORT)$(SCRIPT_NAME)?" */
  /* the returned string should be freed once we're done with it. */
  if ((script_url=msOWSGetOnlineResource2(map, "MO", "onlineresource", req, validated_language)) == NULL ||
      (script_url_encoded = msEncodeHTMLEntities(script_url)) == NULL) {
    msFree(validated_language);
    msFree(schemalocation);
    msFree(dtd_url);
    return msWMSException(map, nVersion, NULL, wms_exception_format);
  }

  if (nVersion <= OWS_1_0_7 || nVersion >= OWS_1_3_0)   /* 1.0.0 to 1.0.7 and >=1.3.0*/
    msIO_setHeader("Content-Type","text/xml; charset=UTF-8");
  else /* 1.1.0 and later */
    msIO_setHeader("Content-Type","application/vnd.ogc.wms_xml; charset=UTF-8");
  msIO_sendHeaders();

  msIO_printf("<?xml version='1.0' encoding=\"UTF-8\" standalone=\"no\" ?>\n");

  /*TODO review wms1.3.0*/
  if ( nVersion < OWS_1_3_0) {
    msIO_printf("<!DOCTYPE WMT_MS_Capabilities SYSTEM \"%s\"\n", dtd_url);
    msIO_printf(" [\n");

    if (nVersion == OWS_1_1_1 && msOWSLookupMetadata(&(map->web.metadata), "MO", "inspire_capabilities") ) {
      msIO_printf("<!ELEMENT VendorSpecificCapabilities (inspire_vs:ExtendedCapabilities)><!ELEMENT inspire_vs:ExtendedCapabilities ((inspire_common:MetadataUrl, inspire_common:SupportedLanguages, inspire_common:ResponseLanguage) | (inspire_common:ResourceLocator+, inspire_common:ResourceType, inspire_common:TemporalReference+, inspire_common:Conformity+, inspire_common:MetadataPointOfContact+, inspire_common:MetadataDate, inspire_common:SpatialDataServiceType, inspire_common:MandatoryKeyword+, inspire_common:Keyword*, inspire_common:SupportedLanguages, inspire_common:ResponseLanguage, inspire_common:MetadataUrl?))><!ATTLIST inspire_vs:ExtendedCapabilities xmlns:inspire_vs CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/inspire_vs/1.0\" ><!ELEMENT inspire_common:MetadataUrl (inspire_common:URL, inspire_common:MediaType*)><!ATTLIST inspire_common:MetadataUrl xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\" xmlns:xsi CDATA #FIXED \"http://www.w3.org/2001/XMLSchema-instance\" xsi:type CDATA #FIXED \"inspire_common:resourceLocatorType\" ><!ELEMENT inspire_common:URL (#PCDATA)><!ATTLIST inspire_common:URL xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\"><!ELEMENT inspire_common:MediaType (#PCDATA)><!ATTLIST inspire_common:MediaType xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\"><!ELEMENT inspire_common:SupportedLanguages (inspire_common:DefaultLanguage, inspire_common:SupportedLanguage*)><!ATTLIST inspire_common:SupportedLanguages xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\" ><!ELEMENT inspire_common:DefaultLanguage (inspire_common:Language)><!ATTLIST inspire_common:DefaultLanguage xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\" ><!ELEMENT inspire_common:SupportedLanguage (inspire_common:Language)><!ATTLIST inspire_common:SupportedLanguage xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\" >"
                  "<!ELEMENT inspire_common:ResponseLanguage (inspire_common:Language)><!ATTLIST inspire_common:ResponseLanguage xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\" ><!ELEMENT inspire_common:Language (#PCDATA)><!ATTLIST inspire_common:Language xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\" ><!ELEMENT inspire_common:ResourceLocator (inspire_common:URL, inspire_common:MediaType*)><!ATTLIST inspire_common:ResourceLocator xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\"><!ELEMENT inspire_common:ResourceType (#PCDATA)> <!ATTLIST inspire_common:ResourceType xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\" ><!ELEMENT inspire_common:TemporalReference (inspire_common:DateOfCreation?, inspire_common:DateOfLastRevision?, inspire_common:DateOfPublication*, inspire_common:TemporalExtent*)><!ATTLIST inspire_common:TemporalReference xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\" ><!ELEMENT inspire_common:DateOfCreation (#PCDATA)> <!ATTLIST inspire_common:DateOfCreation xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\"><!ELEMENT inspire_common:DateOfLastRevision (#PCDATA)><!ATTLIST inspire_common:DateOfLastRevision xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\"><!ELEMENT inspire_common:DateOfPublication (#PCDATA)><!ATTLIST inspire_common:DateOfPublication xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\"><!ELEMENT inspire_common:TemporalExtent (inspire_common:IndividualDate | inspire_common:IntervalOfDates)><!ATTLIST inspire_common:TemporalExtent xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\" ><!ELEMENT inspire_common:IndividualDate (#PCDATA)> <!ATTLIST inspire_common:IndividualDate xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\">"
                  "<!ELEMENT inspire_common:IntervalOfDates (inspire_common:StartingDate, inspire_common:EndDate)><!ATTLIST inspire_common:IntervalOfDates xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\" ><!ELEMENT inspire_common:StartingDate (#PCDATA)><!ATTLIST inspire_common:StartingDate xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\" ><!ELEMENT inspire_common:EndDate (#PCDATA)><!ATTLIST inspire_common:EndDate xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\" ><!ELEMENT inspire_common:Conformity (inspire_common:Specification, inspire_common:Degree)><!ATTLIST inspire_common:Conformity xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\" ><!ELEMENT inspire_common:Specification (inspire_common:Title, (inspire_common:DateOfPublication | inspire_common:DateOfCreation | inspire_common:DateOfLastRevision), inspire_common:URI*, inspire_common:ResourceLocator*)><!ATTLIST inspire_common:Specification xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\" ><!ELEMENT inspire_common:Title (#PCDATA)><!ATTLIST inspire_common:Title xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\" ><!ELEMENT inspire_common:URI (#PCDATA)><!ATTLIST inspire_common:URI xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\" ><!ELEMENT inspire_common:Degree (#PCDATA)><!ATTLIST inspire_common:Degree xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\" ><!ELEMENT inspire_common:MetadataPointOfContact (inspire_common:OrganisationName, inspire_common:EmailAddress)><!ATTLIST inspire_common:MetadataPointOfContact xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\" >"
                  "<!ELEMENT inspire_common:OrganisationName (#PCDATA)><!ATTLIST inspire_common:OrganisationName  xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\" ><!ELEMENT inspire_common:EmailAddress (#PCDATA)><!ATTLIST inspire_common:EmailAddress xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\" ><!ELEMENT inspire_common:MetadataDate (#PCDATA)><!ATTLIST inspire_common:MetadataDate xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\" ><!ELEMENT inspire_common:SpatialDataServiceType (#PCDATA)><!ATTLIST inspire_common:SpatialDataServiceType xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\" ><!ELEMENT inspire_common:MandatoryKeyword (inspire_common:KeywordValue)><!ATTLIST inspire_common:MandatoryKeyword xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\" ><!ELEMENT inspire_common:KeywordValue (#PCDATA)><!ATTLIST inspire_common:KeywordValue xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\" >"
                  "<!ELEMENT inspire_common:Keyword (inspire_common:OriginatingControlledVocabulary?, inspire_common:KeywordValue)><!ATTLIST inspire_common:Keyword xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\" xmlns:xsi CDATA #FIXED \"http://www.w3.org/2001/XMLSchemainstance\" xsi:type (inspire_common:inspireTheme_bul | inspire_common:inspireTheme_cze | inspire_common:inspireTheme_dan | inspire_common:inspireTheme_dut | inspire_common:inspireTheme_eng | inspire_common:inspireTheme_est | inspire_common:inspireTheme_fin | inspire_common:inspireTheme_fre | inspire_common:inspireTheme_ger | inspire_common:inspireTheme_gre | inspire_common:inspireTheme_hun | inspire_common:inspireTheme_gle | inspire_common:inspireTheme_ita | inspire_common:inspireTheme_lav | inspire_common:inspireTheme_lit | inspire_common:inspireTheme_mlt | inspire_common:inspireTheme_pol | inspire_common:inspireTheme_por | inspire_common:inspireTheme_rum | inspire_common:inspireTheme_slo | inspire_common:inspireTheme_slv | inspire_common:inspireTheme_spa | inspire_common:inspireTheme_swe) #IMPLIED ><!ELEMENT inspire_common:OriginatingControlledVocabulary (inspire_common:Title, (inspire_common:DateOfPublication | inspire_common:DateOfCreation | inspire_common:DateOfLastRevision), inspire_common:URI*, inspire_common:ResourceLocator*)><!ATTLIST inspire_common:OriginatingControlledVocabulary xmlns:inspire_common CDATA #FIXED \"http://inspire.ec.europa.eu/schemas/common/1.0\">\n");
    } else {
      /* some mapserver specific declarations will go here */
      msIO_printf(" <!ELEMENT VendorSpecificCapabilities EMPTY>\n");
    }

    msIO_printf(" ]>  <!-- end of DOCTYPE declaration -->\n\n");
  }

  updatesequence = msOWSLookupMetadata(&(map->web.metadata), "MO", "updatesequence");
  if ( nVersion >= OWS_1_3_0)
    msIO_printf("<WMS_Capabilities version=\"%s\"",
                msOWSGetVersionString(nVersion, szVersionBuf));
  else
    msIO_printf("<WMT_MS_Capabilities version=\"%s\"",
                msOWSGetVersionString(nVersion, szVersionBuf));
  if (updatesequence)
    msIO_printf(" updateSequence=\"%s\"",updatesequence);


  if ( nVersion == OWS_1_3_0) {
    msIO_printf("  xmlns=\"http://www.opengis.net/wms\""
                "   xmlns:sld=\"http://www.opengis.net/sld\""
                "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
                "   xmlns:ms=\"http://mapserver.gis.umn.edu/mapserver\"");

    if ( msOWSLookupMetadata(&(map->web.metadata), "MO", "inspire_capabilities") ) {
      msIO_printf("   xmlns:" MS_INSPIRE_COMMON_NAMESPACE_PREFIX "=\"" MS_INSPIRE_COMMON_NAMESPACE_URI "\""
                  "   xmlns:" MS_INSPIRE_VS_NAMESPACE_PREFIX "=\"" MS_INSPIRE_VS_NAMESPACE_URI "\"" );
    }

    msIO_printf("   xsi:schemaLocation=\"http://www.opengis.net/wms %s/wms/%s/capabilities_1_3_0.xsd "
                " http://www.opengis.net/sld %s/sld/1.1.0/sld_capabilities.xsd ",
                msOWSGetSchemasLocation(map), msOWSGetVersionString(nVersion, szVersionBuf), msOWSGetSchemasLocation(map));

    if ( msOWSLookupMetadata(&(map->web.metadata), "MO", "inspire_capabilities") ) {
      char* inspireschemalocation = msEncodeHTMLEntities( msOWSGetInspireSchemasLocation(map) );
      msIO_printf(" " MS_INSPIRE_VS_NAMESPACE_URI " "
                  " %s%s", inspireschemalocation, MS_INSPIRE_VS_SCHEMA_LOCATION);
      free(inspireschemalocation);
    }

    msIO_printf(" http://mapserver.gis.umn.edu/mapserver %sservice=WMS&amp;version=1.3.0&amp;request=GetSchemaExtension\"",
                script_url_encoded);
  }

  msIO_printf(">\n");



  /* Report MapServer Version Information */
  msIO_printf("\n<!-- %s -->\n\n", msGetVersion());

  /* WMS definition */
  msIO_printf("<Service>\n");

  /* Service name is defined by the spec and changed at v1.0.0 */
  if (nVersion <= OWS_1_0_7)
    msIO_printf("  <Name>GetMap</Name>\n");  /* v 1.0.0 to 1.0.7 */
  else if (nVersion > OWS_1_0_7 && nVersion < OWS_1_3_0)
    msIO_printf("  <Name>OGC:WMS</Name>\n"); /* v 1.1.0 to 1.1.1*/
  else
    msIO_printf("  <Name>WMS</Name>\n"); /* v 1.3.0+ */


  /* the majority of this section is dependent on appropriately named metadata in the WEB object */
  msOWSPrintEncodeMetadata2(stdout, &(map->web.metadata), "MO", "title",
                            OWS_WARN, "  <Title>%s</Title>\n", map->name, validated_language);
  msOWSPrintEncodeMetadata2(stdout, &(map->web.metadata), "MO", "abstract",
                            OWS_NOERR, "  <Abstract>%s</Abstract>\n", NULL, validated_language);

  msWMSPrintKeywordlist(stdout, "  ", "keywordlist", &(map->web.metadata), "MO", nVersion);

  /* Service/onlineresource */
  /* Defaults to same as request onlineresource if wms_service_onlineresource  */
  /* is not set. */
  if (nVersion== OWS_1_0_0)
    msOWSPrintEncodeMetadata(stdout, &(map->web.metadata),
                             "MO", "service_onlineresource", OWS_NOERR,
                             "  <OnlineResource>%s</OnlineResource>\n",
                             script_url_encoded);
  else
    msOWSPrintEncodeMetadata(stdout, &(map->web.metadata),
                             "MO", "service_onlineresource", OWS_NOERR,
                             "  <OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:href=\"%s\"/>\n",
                             script_url_encoded);

  /* contact information is a required element in 1.0.7 but the */
  /* sub-elements such as ContactPersonPrimary, etc. are not! */
  /* In 1.1.0, ContactInformation becomes optional. */
  msOWSPrintContactInfo(stdout, "  ", nVersion, &(map->web.metadata), "MO");

  msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "MO", "fees",
                           OWS_NOERR, "  <Fees>%s</Fees>\n", NULL);

  msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "MO",
                           "accessconstraints", OWS_NOERR,
                           "  <AccessConstraints>%s</AccessConstraints>\n", NULL);

  if (nVersion >= OWS_1_3_0) {
    layerlimit = msOWSLookupMetadata(&(map->web.metadata), "MO", "layerlimit");
    if (layerlimit) {
      msIO_printf("  <LayerLimit>%s</LayerLimit>\n", layerlimit);
    }
    msIO_printf("  <MaxWidth>%d</MaxWidth>\n", map->maxsize);
    msIO_printf("  <MaxHeight>%d</MaxHeight>\n", map->maxsize);
  }

  msIO_printf("</Service>\n\n");

  /* WMS capabilities definitions */
  msIO_printf("<Capability>\n");
  msIO_printf("  <Request>\n");

  if (nVersion <= OWS_1_0_7) {
    /* WMS 1.0.0 to 1.0.7 - We don't try to use outputformats list here for now */
    if (msOWSRequestIsEnabled(map, NULL, "M", "GetMap", MS_FALSE))
      msWMSPrintRequestCap(nVersion, "Map", script_url_encoded, ""

#if defined USE_PNG
                           "<PNG />"
#endif
#if defined USE_JPEG
                           "<JPEG />"
#endif
                           "<SVG />"
                           , NULL);
    if (msOWSRequestIsEnabled(map, NULL, "M", "GetCapabilities", MS_TRUE))
      msWMSPrintRequestCap(nVersion, "Capabilities", script_url_encoded, "<WMS_XML />", NULL);
    if (msOWSRequestIsEnabled(map, NULL, "M", "GetFeatureInfo", MS_FALSE))
      msWMSPrintRequestCap(nVersion, "FeatureInfo", script_url_encoded, "<MIME /><GML.1 />", NULL);
  } else {
    char *mime_list[20];
    int mime_count = 0;
    int max_mime = 20;
    /* WMS 1.1.0 and later */
    /* Note changes to the request names, their ordering, and to the formats */

    if (msOWSRequestIsEnabled(map, NULL, "M", "GetCapabilities", MS_TRUE)) {
      if (nVersion >= OWS_1_3_0)
        msWMSPrintRequestCap(nVersion, "GetCapabilities", script_url_encoded,
                             "text/xml",
                             NULL);
      else
        msWMSPrintRequestCap(nVersion, "GetCapabilities", script_url_encoded,
                             "application/vnd.ogc.wms_xml",
                             NULL);
    }

    msGetOutputFormatMimeListWMS(map,mime_list,sizeof(mime_list)/sizeof(char*));
    if (msOWSRequestIsEnabled(map, NULL, "M", "GetMap", MS_FALSE))
      msWMSPrintRequestCap(nVersion, "GetMap", script_url_encoded,
                           mime_list[0], mime_list[1], mime_list[2], mime_list[3],
                           mime_list[4], mime_list[5], mime_list[6], mime_list[7],
                           mime_list[8], mime_list[9], mime_list[10], mime_list[11],
                           mime_list[12], mime_list[13], mime_list[14], mime_list[15],
                           mime_list[16], mime_list[17], mime_list[18], mime_list[19],
                           NULL );

    format_list = msOWSLookupMetadata(&(map->web.metadata), "M",
                                      "getfeatureinfo_formatlist");
    /*feature_info_mime_type depricated for MapServer 6.0*/
    if (!format_list)
      format_list = msOWSLookupMetadata(&(map->web.metadata), "MO",
                                        "feature_info_mime_type");

    if (format_list && strlen(format_list) > 0) {
      tokens = msStringSplit(format_list,  ',', &numtokens);
      if (tokens && numtokens > 0) {
        mime_count = 0;
        for(i=0; i < numtokens; i++ ) {
          msStringTrim(tokens[i]);
          /*text plain and gml do not need to be a format and accepted by default*/
          /*can not really validate since the old way of using template
            with wei->header, layer->template ... should be kept*/
          if (strlen(tokens[i]) > 0 && mime_count<max_mime)
            mime_list[mime_count++] = tokens[i];
        }
      }
      /*add text/plain and gml */
      if (strcasestr(format_list, "GML") == 0 &&
          strcasestr(format_list, "application/vnd.ogc.gml") == 0) {
        if (mime_count<max_mime)
          mime_list[mime_count++] = "application/vnd.ogc.gml";
      }
      if (strcasestr(format_list, "text/plain") == 0 &&
          strcasestr(format_list, "MIME") == 0) {
        if (mime_count<max_mime)
          mime_list[mime_count++] = "text/plain";
        else /*force always this format*/
          mime_list[max_mime-1] = "text/plain";
      }


      if (msOWSRequestIsEnabled(map, NULL, "M", "GetFeatureInfo", MS_FALSE)) {
        if (mime_count>0) {
          if (mime_count<max_mime)
            mime_list[mime_count] = NULL;
          msWMSPrintRequestCap(nVersion, "GetFeatureInfo", script_url_encoded,
                               mime_list[0], mime_list[1], mime_list[2], mime_list[3],
                               mime_list[4], mime_list[5], mime_list[6], mime_list[7],
                               mime_list[8], mime_list[9], mime_list[10], mime_list[11],
                               mime_list[12], mime_list[13], mime_list[14], mime_list[15],
                               mime_list[16], mime_list[17], mime_list[18], mime_list[19],
                               NULL);
        }
        /*if all formats given are invalid go to default*/
        else
          msWMSPrintRequestCap(nVersion, "GetFeatureInfo", script_url_encoded,
                               "text/plain",
                               "application/vnd.ogc.gml",
                               NULL);
      }

      if (numtokens>0)
        msFreeCharArray(tokens, numtokens);
    } else {
      if (msOWSRequestIsEnabled(map, NULL, "M", "GetFeatureInfo", MS_FALSE))
        msWMSPrintRequestCap(nVersion, "GetFeatureInfo", script_url_encoded,
                             "text/plain",
                             "application/vnd.ogc.gml",
                             NULL);
    }


    if (strcasecmp(sldenabled, "true") == 0) {
      if (msOWSRequestIsEnabled(map, NULL, "M", "DescribeLayer", MS_FALSE)) {
        if (nVersion == OWS_1_3_0)
          msWMSPrintRequestCap(nVersion, "sld:DescribeLayer", script_url_encoded, "text/xml", NULL);
        else
          msWMSPrintRequestCap(nVersion, "DescribeLayer", script_url_encoded, "text/xml", NULL);
      }

      msGetOutputFormatMimeListImg(map,mime_list,sizeof(mime_list)/sizeof(char*));

      if (nVersion >= OWS_1_1_1) {
        if (nVersion == OWS_1_3_0) {
          if (msOWSRequestIsEnabled(map, NULL, "M", "GetLegendGraphic", MS_FALSE))
            msWMSPrintRequestCap(nVersion, "sld:GetLegendGraphic", script_url_encoded,
                                 mime_list[0], mime_list[1], mime_list[2], mime_list[3],
                                 mime_list[4], mime_list[5], mime_list[6], mime_list[7],
                                 mime_list[8], mime_list[9], mime_list[10], mime_list[11],
                                 mime_list[12], mime_list[13], mime_list[14], mime_list[15],
                                 mime_list[16], mime_list[17], mime_list[18], mime_list[19],
                                 NULL );
          if (msOWSRequestIsEnabled(map, NULL, "M", "GetStyles", MS_FALSE))
            msWMSPrintRequestCap(nVersion, "ms:GetStyles", script_url_encoded, "text/xml", NULL);
        } else {
          if (msOWSRequestIsEnabled(map, NULL, "M", "GetLegendGraphic", MS_FALSE))
            msWMSPrintRequestCap(nVersion, "GetLegendGraphic", script_url_encoded,
                                 mime_list[0], mime_list[1], mime_list[2], mime_list[3],
                                 mime_list[4], mime_list[5], mime_list[6], mime_list[7],
                                 mime_list[8], mime_list[9], mime_list[10], mime_list[11],
                                 mime_list[12], mime_list[13], mime_list[14], mime_list[15],
                                 mime_list[16], mime_list[17], mime_list[18], mime_list[19],
                                 NULL );
          if (msOWSRequestIsEnabled(map, NULL, "M", "GetStyles", MS_FALSE))
            msWMSPrintRequestCap(nVersion, "GetStyles", script_url_encoded, "text/xml", NULL);
        }
      }
    }
  }

  msIO_printf("  </Request>\n");

  msIO_printf("  <Exception>\n");
  if (nVersion <= OWS_1_0_7)
    msIO_printf("    <Format><BLANK /><INIMAGE /><WMS_XML /></Format>\n");
  else if (nVersion <= OWS_1_1_1) {
    msIO_printf("    <Format>application/vnd.ogc.se_xml</Format>\n");
    msIO_printf("    <Format>application/vnd.ogc.se_inimage</Format>\n");
    msIO_printf("    <Format>application/vnd.ogc.se_blank</Format>\n");
  } else { /*>=1.3.0*/
    msIO_printf("    <Format>XML</Format>\n");
    msIO_printf("    <Format>INIMAGE</Format>\n");
    msIO_printf("    <Format>BLANK</Format>\n");

  }
  msIO_printf("  </Exception>\n");

  if (nVersion != OWS_1_3_0) {
    /* INSPIRE extended capabilities for WMS 1.1.1 */
    if (nVersion == OWS_1_1_1 && msOWSLookupMetadata(&(map->web.metadata), "MO", "inspire_capabilities") ) {
      msIO_printf("  <VendorSpecificCapabilities>\n");
      msOWSPrintInspireCommonExtendedCapabilities(stdout, map, "MO", OWS_WARN, "inspire_vs:ExtendedCapabilities", NULL, validated_language, OWS_WMS);
      msIO_printf("  </VendorSpecificCapabilities>\n");
    } else {
      msIO_printf("  <VendorSpecificCapabilities />\n"); /* nothing yet */
    }
  }

  /* SLD support */
  if (strcasecmp(sldenabled, "true") == 0) {
    if (nVersion >= OWS_1_0_7) {
      if (nVersion >= OWS_1_3_0)
        msIO_printf("  <sld:UserDefinedSymbolization SupportSLD=\"1\" UserLayer=\"0\" UserStyle=\"1\" RemoteWFS=\"0\" InlineFeature=\"0\" RemoteWCS=\"0\"/>\n");
      else
        msIO_printf("  <UserDefinedSymbolization SupportSLD=\"1\" UserLayer=\"0\" UserStyle=\"1\" RemoteWFS=\"0\"/>\n");
    }
  }

  /* INSPIRE extended capabilities for WMS 1.3.0 */
  if (nVersion >= OWS_1_3_0 && msOWSLookupMetadata(&(map->web.metadata), "MO", "inspire_capabilities") ) {
    msOWSPrintInspireCommonExtendedCapabilities(stdout, map, "MO", OWS_WARN, "inspire_vs:ExtendedCapabilities", NULL, validated_language, OWS_WMS);
  }

  /* Top-level layer with map extents and SRS, encloses all map layers */
  /* Output only if at least one layers is enabled. */
  if (ows_request->numlayers == 0) {
    msIO_fprintf(stdout, "  <!-- WARNING: No WMS layers are enabled. Check wms/ows_enable_request settings. -->\n");
  } else {
    int root_is_queryable = MS_FALSE;

    rootlayer_name = msOWSLookupMetadata(&(map->web.metadata), "MO", "rootlayer_name");

    /* Root layer is queryable if it has a valid name and at list one layer */
    /* is queryable */
    if( !rootlayer_name || strlen(rootlayer_name) > 0 ) {
      int j;
      for(j=0; j<map->numlayers; j++) {
        layerObj* layer = GET_LAYER(map, j);
        if( msIsLayerQueryable(layer) &&
            msIntegerInArray(layer->index, ows_request->enabled_layers, ows_request->numlayers) ) {
          root_is_queryable = MS_TRUE;
          break;
        }
      }
    }
    msIO_printf("  <Layer%s>\n",root_is_queryable ? " queryable=\"1\"" : "");

    /* Layer Name is optional but title is mandatory. */
    if (map->name && strlen(map->name) > 0 &&
        (msIsXMLTagValid(map->name) == MS_FALSE || isdigit(map->name[0])))
      msIO_fprintf(stdout, "<!-- WARNING: The layer name '%s' might contain spaces or "
                   "invalid characters or may start with a number. This could lead to potential problems. -->\n",
                   map->name);

    if (rootlayer_name) {
        if (strlen(rootlayer_name) > 0) {
            msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "MO", "rootlayer_name",
                                     OWS_NOERR, "    <Name>%s</Name>\n",
                                     NULL);
        }
    }
    else {
        msOWSPrintEncodeParam(stdout, "MAP.NAME", map->name, OWS_NOERR,
                              "    <Name>%s</Name>\n", NULL);
    }

    if (msOWSLookupMetadataWithLanguage(&(map->web.metadata), "MO", "rootlayer_title", validated_language))
      msOWSPrintEncodeMetadata2(stdout, &(map->web.metadata), "MO", "rootlayer_title", OWS_WARN, "    <Title>%s</Title>\n", map->name, validated_language);

    else
      msOWSPrintEncodeMetadata2(stdout, &(map->web.metadata), "MO", "title", OWS_WARN, "    <Title>%s</Title>\n", map->name, validated_language);

    if (msOWSLookupMetadataWithLanguage(&(map->web.metadata), "MO", "rootlayer_abstract", validated_language))
      msOWSPrintEncodeMetadata2(stdout, &(map->web.metadata), "MO", "rootlayer_abstract", OWS_NOERR, "    <Abstract>%s</Abstract>\n", map->name, validated_language);
    else
      msOWSPrintEncodeMetadata2(stdout, &(map->web.metadata), "MO", "abstract", OWS_NOERR, "    <Abstract>%s</Abstract>\n", map->name, validated_language);

    if (msOWSLookupMetadata(&(map->web.metadata), "MO", "rootlayer_keywordlist") ||
        msOWSLookupMetadata(&(map->web.metadata), "MO", "rootlayer_keywordlist_vocabulary"))
      pszTmp = msStrdup("rootlayer_keywordlist");
    else
      pszTmp = msStrdup("keywordlist");
    msWMSPrintKeywordlist(stdout, "    ", pszTmp, &(map->web.metadata), "MO", nVersion);
    msFree(pszTmp);

    /* According to normative comments in the 1.0.7 DTD, the root layer's SRS tag */
    /* is REQUIRED.  It also suggests that we use an empty SRS element if there */
    /* is no common SRS. */
    msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), "MO", MS_FALSE, &pszMapEPSG);
    if (nVersion > OWS_1_1_0) {
      /* starting 1.1.1 SRS are given in individual tags */
      if (nVersion >= OWS_1_3_0) {
        msOWSPrintEncodeParamList(stdout, "(at least one of) "
                                  "MAP.PROJECTION, LAYER.PROJECTION "
                                  "or wms_srs metadata",
                                  pszMapEPSG,
                                  OWS_WARN, ' ', NULL, NULL,
                                  "    <CRS>%s</CRS>\n", "");
      } else {
        msOWSPrintEncodeParamList(stdout, "(at least one of) "
                                  "MAP.PROJECTION, LAYER.PROJECTION "
                                  "or wms_srs metadata",
                                  pszMapEPSG,
                                  OWS_WARN, ' ', NULL, NULL,
                                  "    <SRS>%s</SRS>\n", "");
      }
    } else {
      /* If map has no proj then every layer MUST have one or produce a warning */
      msOWSPrintEncodeParam(stdout, "MAP.PROJECTION (or wms_srs metadata)",
                            pszMapEPSG,
                            OWS_WARN, "    <SRS>%s</SRS>\n", "");
    }
    msFree(pszMapEPSG);

    if (nVersion >= OWS_1_3_0)
      msOWSPrintEX_GeographicBoundingBox(stdout, "    ", &(map->extent),
                                         &(map->projection));
    else
      msOWSPrintLatLonBoundingBox(stdout, "    ", &(map->extent),
                                  &(map->projection), NULL, OWS_WMS);

    msOWSPrintBoundingBox( stdout, "    ", &(map->extent), &(map->projection),
                           NULL, &(map->web.metadata), "MO", nVersion);

    if (nVersion >= OWS_1_0_7) {
      msWMSPrintAttribution(stdout, "    ", &(map->web.metadata), "MO");
    }

    /* AuthorityURL support and Identifier support, only available >= WMS 1.1.0 */
    if(nVersion >= OWS_1_1_0) {
      msWMSPrintAuthorityURL(stdout, "    ", &(map->web.metadata), "MO");
      msWMSPrintIdentifier(stdout, "    ", &(map->web.metadata), "MO");
    }

    /* MetadataURL */
    if(nVersion >= OWS_1_1_0)
      msOWSPrintURLType(stdout, &(map->web.metadata), "MO", "metadataurl",
                        OWS_NOERR, NULL, "MetadataURL", " type=\"%s\"",
                        NULL, NULL, ">\n      <Format>%s</Format",
                        "\n      <OnlineResource xmlns:xlink=\""
                        "http://www.w3.org/1999/xlink\" "
                        "xlink:type=\"simple\" xlink:href=\"%s\"/>\n    ",
                        MS_TRUE, MS_FALSE, MS_FALSE, MS_TRUE, MS_TRUE,
                        NULL, NULL, NULL, NULL, NULL, "    ");

    /* DataURL */
    if(nVersion < OWS_1_1_0)
      msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "MO", "dataurl_href",
                               OWS_NOERR, "    <DataURL>%s</DataURL>\n",
                               NULL);
    else
      msOWSPrintURLType(stdout, &(map->web.metadata), "MO", "dataurl",
                        OWS_NOERR, NULL, "DataURL", NULL, NULL, NULL,
                        ">\n      <Format>%s</Format",
                        "\n      <OnlineResource xmlns:xlink=\""
                        "http://www.w3.org/1999/xlink\" "
                        "xlink:type=\"simple\" xlink:href=\"%s\"/>\n    ",
                        MS_FALSE, MS_FALSE, MS_FALSE, MS_TRUE, MS_TRUE,
                        NULL, NULL, NULL, NULL, NULL, "    ");

    if (map->name && strlen(map->name) > 0 && msOWSLookupMetadata(&(map->web.metadata), "MO", "inspire_capabilities") ) {
      char *pszEncodedName = NULL;
      const char *styleName = NULL;
      char *pszEncodedStyleName = NULL;
      const char *legendURL = NULL;

      pszEncodedName = msEncodeHTMLEntities(map->name);

      styleName = msOWSLookupMetadata(&(map->web.metadata), "MO", "style_name");
      if (styleName == NULL)
        styleName = "default";

      pszEncodedStyleName = msEncodeHTMLEntities(styleName);

      msIO_fprintf(stdout, "    <Style>\n");
      msIO_fprintf(stdout, "       <Name>%s</Name>\n", pszEncodedStyleName);
      msOWSPrintEncodeMetadata2(stdout, &(map->web.metadata), "MO",
                                "style_title",
                                OWS_NOERR,
                                "       <Title>%s</Title>\n",
                                styleName, validated_language);

      legendURL = msOWSLookupMetadata(&(map->web.metadata), "MO", "style_legendurl_href");
      if (legendURL) {
        msOWSPrintURLType(stdout, &(map->web.metadata),
                          "MO", "style_legendurl",
                          OWS_NOERR, NULL,
                          "LegendURL", NULL,
                          " width=\"%s\"", " height=\"%s\"",
                          ">\n          <Format>%s</Format",
                          "\n          <OnlineResource "
                          "xmlns:xlink=\"http://www.w3.org/1999/xlink\""
                          " xlink:type=\"simple\" xlink:href=\"%s\"/>\n"
                          "       ",
                          MS_FALSE, MS_TRUE, MS_TRUE, MS_TRUE, MS_TRUE,
                          NULL, NULL, NULL, NULL, NULL, "       ");
      } else {
        int i, num_layers = 0;
        int *group_layers = (int *)msSmallMalloc(sizeof(int)*map->numlayers);

        for(i=0; i<map->numlayers; i++)
          if (msIntegerInArray(GET_LAYER(map, i)->index, ows_request->enabled_layers, ows_request->numlayers))
            group_layers[num_layers++] = i;

        if (num_layers > 0) {
          int size_x=0, size_y=0;
          size_t bufferSize = 0;
          char *pszLegendURL = NULL;
          char *pszMimetype = NULL;
          char **tokens = NULL;
          int numtokens = 0;
          char width[10], height[10];

          group_layers =(int *)msSmallRealloc(group_layers, sizeof(int)*num_layers);
          if (msLegendCalcSize(map, 1, &size_x, &size_y,  group_layers , num_layers, NULL, 1) == MS_SUCCESS) {
            bufferSize = strlen(script_url_encoded)+300;
            pszLegendURL = (char*)msSmallMalloc(bufferSize);
            snprintf(width, sizeof(width), "%d", size_x);
            snprintf(height, sizeof(height), "%d", size_y);

            format_list = msOWSLookupMetadata(&(map->web.metadata), "M", "getlegendgraphic_formatlist");
            if (format_list && strlen(format_list) > 0) {
              tokens = msStringSplit(format_list,  ',', &numtokens);
              if (tokens && numtokens > 0) {
                /*just grab the first mime type*/
                pszMimetype = msEncodeHTMLEntities(tokens[0]);
                msFreeCharArray(tokens, numtokens);
              }
            } else
              pszMimetype = msEncodeHTMLEntities("image/png");

            if (nVersion >= OWS_1_3_0)
              snprintf(pszLegendURL, bufferSize, "%sversion=%s&amp;service=WMS&amp;request=GetLegendGraphic&amp;sld_version=1.1.0&amp;layer=%s&amp;format=%s&amp;STYLE=%s", script_url_encoded,msOWSGetVersionString(nVersion, szVersionBuf),pszEncodedName,
                       pszMimetype, pszEncodedStyleName);
            else
              snprintf(pszLegendURL, bufferSize, "%sversion=%s&amp;service=WMS&amp;request=GetLegendGraphic&amp;layer=%s&amp;format=%s&amp;STYLE=%s", script_url_encoded,msOWSGetVersionString(nVersion, szVersionBuf),pszEncodedName,
                       pszMimetype, pszEncodedStyleName);

            msOWSPrintURLType(stdout, NULL,
                              "O", "ttt",
                              OWS_NOERR, NULL,
                              "LegendURL", NULL,
                              " width=\"%s\"", " height=\"%s\"",
                              ">\n          <Format>%s</Format",
                              "\n          <OnlineResource "
                              "xmlns:xlink=\"http://www.w3.org/1999/xlink\""
                              " xlink:type=\"simple\" xlink:href=\"%s\"/>\n"
                              "       ",
                              MS_FALSE, MS_FALSE, MS_FALSE, MS_FALSE, MS_FALSE,
                              NULL, width, height, pszMimetype, pszLegendURL, "       ");
          }
          msFree(pszLegendURL);
          msFree(pszMimetype);
        }
        msFree(group_layers);
      }
      msIO_fprintf(stdout, "    </Style>\n");
      msFree(pszEncodedName);
      msFree(pszEncodedStyleName);

    }

    if (nVersion <  OWS_1_3_0)
      msWMSPrintScaleHint("    ", map->web.minscaledenom, map->web.maxscaledenom,
                          map->resolution);
    else
      msWMSPrintScaleDenominator("    ", map->web.minscaledenom, map->web.maxscaledenom);

    /*  */
    /* Dump list of layers organized by groups.  Layers with no group are listed */
    /* individually, at the same level as the groups in the layer hierarchy */
    /*  */
    if (map->numlayers) {
      int i, j;
      char *pabLayerProcessed = NULL;
      char ***nestedGroups = NULL;
      int *numNestedGroups = NULL;
      int *isUsedInNestedGroup = NULL;

      /* We'll use this array of booleans to track which layer/group have been */
      /* processed already */
      pabLayerProcessed = (char *)msSmallCalloc(map->numlayers, sizeof(char));

      /* Mark disabled layers as processed to prevent from being displayed in nested groups (#4533)*/
      for(i=0; i<map->numlayers; i++) {
        if (!msIntegerInArray(GET_LAYER(map, i)->index, ows_request->enabled_layers, ows_request->numlayers))
          pabLayerProcessed[i] = 1;
      }

      nestedGroups = (char***)msSmallCalloc(map->numlayers, sizeof(char**));
      numNestedGroups = (int*)msSmallCalloc(map->numlayers, sizeof(int));
      isUsedInNestedGroup = (int*)msSmallCalloc(map->numlayers, sizeof(int));
      msWMSPrepareNestedGroups(map, nVersion, nestedGroups, numNestedGroups, isUsedInNestedGroup);

      for(i=0; i<map->numlayers; i++) {
        layerObj *lp;
        lp = (GET_LAYER(map, i));

        if (pabLayerProcessed[i] || (lp->status == MS_DELETE))
          continue;  /* Layer is hidden or has already been handled */

        if (numNestedGroups[i] > 0) {
          /* Has nested groups.  */
          msWMSPrintNestedGroups(map, nVersion, pabLayerProcessed, i, 0,
                                 nestedGroups, numNestedGroups, isUsedInNestedGroup,
                                 script_url_encoded, validated_language);
        } else if (lp->group == NULL || strlen(lp->group) == 0) {
          /* Don't dump layer if it is used in wms_group_layer. */
          if (!isUsedInNestedGroup[i]) {
            /* This layer is not part of a group... dump it directly */
            msDumpLayer(map, lp, nVersion, script_url_encoded, "", validated_language, MS_FALSE, MS_FALSE);
            pabLayerProcessed[i] = 1;
          }
        } else {
          int group_is_queryable = MS_FALSE;
          /* Group is queryable as soon as its member layers is. */
          for(j=i; j<map->numlayers; j++) {
            if (GET_LAYER(map, j)->group &&
                strcmp(lp->group, GET_LAYER(map, j)->group) == 0 &&
                msIntegerInArray(GET_LAYER(map, j)->index, ows_request->enabled_layers, ows_request->numlayers) &&
                msIsLayerQueryable(GET_LAYER(map, j)) ) {
                group_is_queryable = MS_TRUE;
                break;
            }
          }
          /* Beginning of a new group... enclose the group in a layer block */
          msIO_printf("    <Layer%s>\n",
                      group_is_queryable ? " queryable=\"1\"" : "");

          /* Layer Name is optional but title is mandatory. */
          if (lp->group &&  strlen(lp->group) > 0 &&
              (msIsXMLTagValid(lp->group) == MS_FALSE || isdigit(lp->group[0])))
            msIO_fprintf(stdout, "<!-- WARNING: The layer name '%s' might contain spaces or "
                         "invalid characters or may start with a number. This could lead to potential problems. -->\n",
                         lp->group);
          msOWSPrintEncodeParam(stdout, "GROUP.NAME", lp->group,
                                OWS_NOERR, "      <Name>%s</Name>\n", NULL);
          msOWSPrintGroupMetadata2(stdout, map, lp->group,
                                   "MO", "GROUP_TITLE", OWS_WARN,
                                   "      <Title>%s</Title>\n", lp->group, validated_language);
          msOWSPrintGroupMetadata2(stdout, map, lp->group,
                                   "MO", "GROUP_ABSTRACT", OWS_NOERR,
                                   "      <Abstract>%s</Abstract>\n", lp->group, validated_language);

          /*build a getlegendgraphicurl*/
          if( script_url_encoded) {
            if (lp->group && strlen(lp->group) > 0) {
              char *pszEncodedName = NULL;
              const char *styleName = NULL;
              char *pszEncodedStyleName = NULL;
              const char *legendURL = NULL;

              pszEncodedName = msEncodeHTMLEntities(lp->group);

              styleName = msOWSLookupMetadata(&(lp->metadata), "MO", "group_style_name");
              if (styleName == NULL)
                styleName = "default";

              pszEncodedStyleName = msEncodeHTMLEntities(styleName);

              msIO_fprintf(stdout, "    <Style>\n");
              msIO_fprintf(stdout, "       <Name>%s</Name>\n", pszEncodedStyleName);
              msOWSPrintEncodeMetadata(stdout, &(lp->metadata), "MO",
                                       "group_style_title",
                                       OWS_NOERR,
                                       "       <Title>%s</Title>\n",
                                       styleName);

              legendURL = msOWSLookupMetadata(&(lp->metadata), "MO", "group_style_legendurl_href");
              if (legendURL) {
                msOWSPrintURLType(stdout, &(lp->metadata),
                                  "MO", "group_style_legendurl",
                                  OWS_NOERR, NULL,
                                  "LegendURL", NULL,
                                  " width=\"%s\"", " height=\"%s\"",
                                  ">\n          <Format>%s</Format",
                                  "\n          <OnlineResource "
                                  "xmlns:xlink=\"http://www.w3.org/1999/xlink\""
                                  " xlink:type=\"simple\" xlink:href=\"%s\"/>\n"
                                  "       ",
                                  MS_FALSE, MS_TRUE, MS_TRUE, MS_TRUE, MS_TRUE,
                                  NULL, NULL, NULL, NULL, NULL, "       ");
              } else {
                int num_layers = 0;
                int *group_layers = (int *)msSmallMalloc(sizeof(int)*map->numlayers);

                for(j=i; j<map->numlayers; j++)
                  if (!pabLayerProcessed[j] &&
                      GET_LAYER(map, j)->group &&
                      strcmp(lp->group, GET_LAYER(map, j)->group) == 0 &&
                      msIntegerInArray(GET_LAYER(map, j)->index, ows_request->enabled_layers, ows_request->numlayers))
                    group_layers[num_layers++] = j;

                if (num_layers > 0) {
                  int size_x=0, size_y=0;
                  size_t bufferSize = 0;
                  char *pszLegendURL = NULL;
                  char *pszMimetype = NULL;
                  char **tokens = NULL;
                  int numtokens = 0;
                  char width[10], height[10];

                  group_layers =(int *)msSmallRealloc(group_layers,  sizeof(int)*num_layers);
                  if (msLegendCalcSize(map, 1, &size_x, &size_y,  group_layers , num_layers, NULL, 1) == MS_SUCCESS) {
                    bufferSize = strlen(script_url_encoded)+300;
                    pszLegendURL = (char*)msSmallMalloc(bufferSize);
                    snprintf(width, sizeof(width), "%d", size_x);
                    snprintf(height, sizeof(height), "%d", size_y);

                    format_list = msOWSLookupMetadata(&(map->web.metadata), "M", "getlegendgraphic_formatlist");
                    if (format_list && strlen(format_list) > 0) {
                      tokens = msStringSplit(format_list,  ',', &numtokens);
                      if (tokens && numtokens > 0) {
                        /*just grab the first mime type*/
                        pszMimetype = msEncodeHTMLEntities(tokens[0]);
                        msFreeCharArray(tokens, numtokens);
                      }
                    } else
                      pszMimetype = msEncodeHTMLEntities("image/png");

                    if (nVersion >= OWS_1_3_0)
                      snprintf(pszLegendURL, bufferSize, "%sversion=%s&amp;service=WMS&amp;request=GetLegendGraphic&amp;sld_version=1.1.0&amp;layer=%s&amp;format=%s&amp;STYLE=%s", script_url_encoded,msOWSGetVersionString(nVersion, szVersionBuf),pszEncodedName,
                               pszMimetype, pszEncodedStyleName);
                    else
                      snprintf(pszLegendURL, bufferSize, "%sversion=%s&amp;service=WMS&amp;request=GetLegendGraphic&amp;layer=%s&amp;format=%s&amp;STYLE=%s", script_url_encoded,msOWSGetVersionString(nVersion, szVersionBuf),pszEncodedName,
                               pszMimetype, pszEncodedStyleName);

                    msOWSPrintURLType(stdout, NULL,
                                      "O", "ttt",
                                      OWS_NOERR, NULL,
                                      "LegendURL", NULL,
                                      " width=\"%s\"", " height=\"%s\"",
                                      ">\n          <Format>%s</Format",
                                      "\n          <OnlineResource "
                                      "xmlns:xlink=\"http://www.w3.org/1999/xlink\""
                                      " xlink:type=\"simple\" xlink:href=\"%s\"/>\n"
                                      "       ",
                                      MS_FALSE, MS_FALSE, MS_FALSE, MS_FALSE, MS_FALSE,
                                      NULL, width, height, pszMimetype, pszLegendURL, "       ");

                    msFree(pszLegendURL);
                    msFree(pszMimetype);
                  }
                }

                msFree(group_layers);
              }
              msIO_fprintf(stdout, "    </Style>\n");
              msFree(pszEncodedName);
              msFree(pszEncodedStyleName);
            }
          }

          /* Dump all layers for this group */
          for(j=i; j<map->numlayers; j++) {
            if (!pabLayerProcessed[j] &&
                GET_LAYER(map, j)->group &&
                strcmp(lp->group, GET_LAYER(map, j)->group) == 0 &&
                msIntegerInArray(GET_LAYER(map, j)->index, ows_request->enabled_layers, ows_request->numlayers)) {
              msDumpLayer(map, (GET_LAYER(map, j)), nVersion, script_url_encoded, "  ", validated_language, MS_FALSE, MS_FALSE);
              pabLayerProcessed[j] = 1;
            }
          }
          /* Close group layer block */
          msIO_printf("    </Layer>\n");
        }
      }

      free(pabLayerProcessed);

      /* free the stuff used for nested layers */
      for (i = 0; i < map->numlayers; i++) {
        if (numNestedGroups[i] > 0) {
          msFreeCharArray(nestedGroups[i], numNestedGroups[i]);
        }
      }
      free(nestedGroups);
      free(numNestedGroups);
      free(isUsedInNestedGroup);
    }

    msIO_printf("  </Layer>\n");
  }

  msIO_printf("</Capability>\n");
  if ( nVersion >= OWS_1_3_0)
    msIO_printf("</WMS_Capabilities>\n");
  else
    msIO_printf("</WMT_MS_Capabilities>\n");

  msFree(validated_language);

  free(script_url);
  free(script_url_encoded);

  free(dtd_url);
  free(schemalocation);

  return(MS_SUCCESS);
}

/*
 * This function look for params that can be used
 * by mapserv when generating template.
*/
int msTranslateWMS2Mapserv(const char **names, const char **values, int numentries, char ***translated_names, char ***translated_values, int *translated_numentries)
{
  int i=0, num_allocated = numentries;
  *translated_names = (char**)msSmallMalloc(num_allocated * sizeof(char*));
  *translated_values = (char**)msSmallMalloc(num_allocated * sizeof(char*));
  *translated_numentries = 0;
  for (i=0; i<numentries; i++) {
    (*translated_values)[*translated_numentries] = msStrdup(values[i]);
    (*translated_names)[*translated_numentries] = msStrdup(names[i]);
    (*translated_numentries)++;
    if (strcasecmp("X", names[i]) == 0) {
      num_allocated++;
      *translated_names = (char**)msSmallRealloc(*translated_names, num_allocated * sizeof(char*));
      *translated_values = (char**)msSmallRealloc(*translated_values, num_allocated * sizeof(char*));
      (*translated_values)[*translated_numentries] = msStrdup(values[i]);
      (*translated_names)[*translated_numentries] = msStrdup("img.x");
      (*translated_numentries)++;
    } else if (strcasecmp("Y", names[i]) == 0) {
      num_allocated++;
      *translated_names = (char**)msSmallRealloc(*translated_names, num_allocated * sizeof(char*));
      *translated_values = (char**)msSmallRealloc(*translated_values, num_allocated * sizeof(char*));
      (*translated_values)[*translated_numentries] = msStrdup(values[i]);
      (*translated_names)[*translated_numentries] = msStrdup("img.y");
      (*translated_numentries)++;
    } else if (strcasecmp("LAYERS", names[i]) == 0) {
      char **layers;
      int tok;
      int j;
      layers = msStringSplit(values[i], ',', &tok);
      num_allocated += tok;
      *translated_names = (char**)msSmallRealloc(*translated_names, num_allocated * sizeof(char*));
      *translated_values = (char**)msSmallRealloc(*translated_values, num_allocated * sizeof(char*));
      for (j=0; j<tok; j++) {
        (*translated_values)[*translated_numentries] = layers[j];
        (*translated_names)[*translated_numentries] = msStrdup("layer");
        (*translated_numentries)++;
        layers[j] = NULL;
      }
      free(layers);
    } else if (strcasecmp("QUERY_LAYERS", names[i]) == 0) {
      char **layers;
      int tok;
      int j;
      layers = msStringSplit(values[i], ',', &tok);
      num_allocated += tok;
      *translated_names = (char**)msSmallRealloc(*translated_names, num_allocated * sizeof(char*));
      *translated_values = (char**)msSmallRealloc(*translated_values, num_allocated * sizeof(char*));
      for (j=0; j<tok; j++) {
        (*translated_values)[*translated_numentries] = layers[j];
        (*translated_names)[*translated_numentries] = msStrdup("qlayer");
        (*translated_numentries)++;
        layers[j] = NULL;
      }
      free(layers);
    } else if (strcasecmp("BBOX", names[i]) == 0) {
      char *imgext;
      num_allocated++;
      *translated_names = (char**)msSmallRealloc(*translated_names, num_allocated * sizeof(char*));
      *translated_values = (char**)msSmallRealloc(*translated_values, num_allocated * sizeof(char*));

      /* Note msReplaceSubstring() works on the string itself, so we need to make a copy */
      imgext = msStrdup(values[i]);
      imgext = msReplaceSubstring(imgext, ",", " ");
      (*translated_values)[*translated_numentries] = imgext;
      (*translated_names)[*translated_numentries] = msStrdup("imgext");
      (*translated_numentries)++;
    }
  }


  return MS_SUCCESS;
}

/*
** msWMSGetMap()
*/
int msWMSGetMap(mapObj *map, int nVersion, char **names, char **values, int numentries,
                char *wms_exception_format, owsRequestObj *ows_request)
{
  imageObj *img;
  int i = 0;
  int sldrequested = MS_FALSE,  sldspatialfilter = MS_FALSE;
  int drawquerymap = MS_FALSE;
  const char *http_max_age;

  /* __TODO__ msDrawMap() will try to adjust the extent of the map */
  /* to match the width/height image ratio. */
  /* The spec states that this should not happen so that we can deliver */
  /* maps to devices with non-square pixels. */

  /* If there was an SLD in the request, we need to treat it */
  /* diffrently : some SLD may contain spatial filters requiring */
  /* to do a query. While parsing the SLD and applying it to the */
  /* layer, we added a temporary metadata on the layer */
  /* (tmp_wms_sld_query) for layers with a spatial filter. */

  for (i=0; i<numentries; i++) {
    if ((strcasecmp(names[i], "SLD") == 0 && values[i] && strlen(values[i]) > 0) ||
        (strcasecmp(names[i], "SLD_BODY") == 0 && values[i] && strlen(values[i]) > 0)) {
      sldrequested = MS_TRUE;
      break;
    }
  }
  if (sldrequested) {
    for (i=0; i<map->numlayers; i++) {
      if (msLookupHashTable(&(GET_LAYER(map, i)->metadata), "tmp_wms_sld_query")) {
        sldspatialfilter = MS_TRUE;
        break;
      }
    }
  }
  /* If FILTER is passed then we'll render layers as querymap */
  for (i=0; i<numentries; i++) {
    if ((strcasecmp(names[i], "FILTER") == 0 && values[i] && strlen(values[i]) > 0)) {
      drawquerymap = MS_TRUE;
      map->querymap.status = MS_ON;
      map->querymap.style = MS_SELECTED;
      break;
    }
  }

  /* turn off layer if WMS GetMap is not enabled */
  for (i=0; i<map->numlayers; i++)
    if (!msIntegerInArray(GET_LAYER(map, i)->index, ows_request->enabled_layers, ows_request->numlayers))
      GET_LAYER(map, i)->status = MS_OFF;

  if (sldrequested && sldspatialfilter) {
    /* set the quermap style so that only selected features will be retruned */
    map->querymap.status = MS_ON;
    map->querymap.style = MS_SELECTED;

    img = msPrepareImage(map, MS_TRUE);

    /* compute layer scale factors now */
    for(i=0; i<map->numlayers; i++) {
      if(GET_LAYER(map, i)->sizeunits != MS_PIXELS)
        GET_LAYER(map, i)->scalefactor = (msInchesPerUnit(GET_LAYER(map, i)->sizeunits,0)/msInchesPerUnit(map->units,0)) / map->cellsize;
      else if(GET_LAYER(map, i)->symbolscaledenom > 0 && map->scaledenom > 0)
        GET_LAYER(map, i)->scalefactor = GET_LAYER(map, i)->symbolscaledenom/map->scaledenom;
      else
        GET_LAYER(map, i)->scalefactor = 1;
    }
    for (i=0; i<map->numlayers; i++) {
      if (msLookupHashTable(&(GET_LAYER(map, i)->metadata), "tmp_wms_sld_query") &&
          (GET_LAYER(map, i)->type == MS_LAYER_POINT ||
           GET_LAYER(map, i)->type == MS_LAYER_LINE ||
           GET_LAYER(map, i)->type == MS_LAYER_POLYGON ||
           GET_LAYER(map, i)->type == MS_LAYER_TILEINDEX))

      {
        /* make sure that there is a resultcache. If not just ignore */
        /* the layer */
        if (GET_LAYER(map, i)->resultcache)
          msDrawQueryLayer(map, GET_LAYER(map, i), img);
      }

      else
        msDrawLayer(map, GET_LAYER(map, i), img);
    }

  } else {

    /* intercept requests for Mapbox vector tiles */
    if(!strcmp(MS_IMAGE_MIME_TYPE(map->outputformat), "application/x-protobuf")) {
      int status=0;
      if((status = msMVTWriteTile(map, MS_TRUE)) != MS_SUCCESS) return MS_FAILURE;
      return MS_SUCCESS;
    }

    img = msDrawMap(map, drawquerymap);
  }

  if (img == NULL)
    return msWMSException(map, nVersion, NULL, wms_exception_format);

  /* Set the HTTP Cache-control headers if they are defined
     in the map object */

  if( (http_max_age = msOWSLookupMetadata(&(map->web.metadata), "MO", "http_max_age")) ) {
    msIO_setHeader("Cache-Control","max-age=%s", http_max_age);
  }

  if (strcasecmp(map->imagetype, "application/openlayers")!=0) {
    if(!strcmp(MS_IMAGE_MIME_TYPE(map->outputformat), "application/json")) {
      msIO_setHeader("Content-Type","application/json; charset=utf-8");
    } else {
      msOutputFormatResolveFromImage( map, img );
      msIO_setHeader("Content-Type", "%s", MS_IMAGE_MIME_TYPE(map->outputformat));
    }
    msIO_sendHeaders();
    if (msSaveImage(map, img, NULL) != MS_SUCCESS) {
      msFreeImage(img);
      return msWMSException(map, nVersion, NULL, wms_exception_format);
    }
  }
  msFreeImage(img);

  return(MS_SUCCESS);
}

int msDumpResult(mapObj *map, int bFormatHtml, int nVersion, char *wms_exception_format)
{
  int numresults=0;
  int i;

  for(i=0; i<map->numlayers; i++) {
    int j, k, *itemvisible;
    char **incitems=NULL;
    int numincitems=0;
    char **excitems=NULL;
    int numexcitems=0;
    const char *value;
    size_t bufferSize=0;
    size_t reqBuffSize;
    char *tag=NULL;
    const char *lineTemplate="    %s = '%s'\n";

    layerObj *lp;
    lp = (GET_LAYER(map, i));

    if(lp->status != MS_ON || lp->resultcache==NULL || lp->resultcache->numresults == 0)
      continue;

    /* if(msLayerOpen(lp) != MS_SUCCESS || msLayerGetItems(lp) != MS_SUCCESS)
     return msWMSException(map, nVersion, NULL); */

    /* Use metadata to control which fields to output. We use the same
     * metadata names as for GML:
     * wms/ows_include_items: comma delimited list or keyword 'all'
     * wms/ows_exclude_items: comma delimited list (all items are excluded by default)
     */
    /* get a list of items that should be excluded in output */
    if((value = msOWSLookupMetadata(&(lp->metadata), "MO", "include_items")) != NULL)
      incitems = msStringSplit(value, ',', &numincitems);

    /* get a list of items that should be excluded in output */
    if((value = msOWSLookupMetadata(&(lp->metadata), "MO", "exclude_items")) != NULL)
      excitems = msStringSplit(value, ',', &numexcitems);

    itemvisible = (int*)msSmallMalloc(lp->numitems*sizeof(int));
    for(k=0; k<lp->numitems; k++) {
      int l;

      itemvisible[k] = MS_FALSE;

      /* check visibility, included items first... */
      if(numincitems == 1 && strcasecmp("all", incitems[0]) == 0) {
        itemvisible[k] = MS_TRUE;
      } else {
        for(l=0; l<numincitems; l++) {
          if(strcasecmp(lp->items[k], incitems[l]) == 0)
            itemvisible[k] = MS_TRUE;
        }
      }

      /* ...and now excluded items */
      for(l=0; l<numexcitems; l++) {
        if(strcasecmp(lp->items[k], excitems[l]) == 0)
          itemvisible[k] = MS_FALSE;
      }
    }

    msFreeCharArray(incitems, numincitems);
    msFreeCharArray(excitems, numexcitems);

    /* Output selected shapes for this layer */
    msIO_printf("\nLayer '%s'\n", lp->name);

    for(j=0; j<lp->resultcache->numresults; j++) {
      shapeObj shape;

      msInitShape(&shape);
      if (msLayerGetShape(lp, &shape, &(lp->resultcache->results[j])) != MS_SUCCESS) {
        if (tag != NULL) msFree(tag);
        msFree(itemvisible);
        return msWMSException(map, nVersion, NULL, wms_exception_format);
      }

      msIO_printf("  Feature %ld: \n", lp->resultcache->results[j].shapeindex);

      for(k=0; k<lp->numitems; k++) {
        if (itemvisible[k]) {
          reqBuffSize = strlen(lp->items[k]) + 7;
          if (reqBuffSize > bufferSize) {
            if (tag != NULL) msFree(tag);
            /* allocate more buffer than we need to try and avoid need for
               repeated reallocation */
            bufferSize = reqBuffSize * 2;
            tag = (char*)msSmallMalloc(bufferSize);
          }
          snprintf(tag, reqBuffSize, "%s_alias", lp->items[k]);

          if((value = msOWSLookupMetadata(&(lp->metadata), "MO", tag)) != NULL)
            msIO_printf(lineTemplate, value, shape.values[k]);
          else
            msIO_printf(lineTemplate, lp->items[k], shape.values[k]);

        }
      }
 
      msFreeShape(&shape);
      numresults++;
    }

    if (tag != NULL) msFree(tag);
    msFree(itemvisible);

    /* msLayerClose(lp); */
  }

  return numresults;
}


/*
** msWMSFeatureInfo()
*/
int msWMSFeatureInfo(mapObj *map, int nVersion, char **names, char **values, int numentries,
                     char *wms_exception_format, owsRequestObj *ows_request)
{
  int i, feature_count=1, numlayers_found=0;
  pointObj point = {-1.0, -1.0};
  const char *info_format="MIME";
  double cellx, celly;
  errorObj *ms_error = msGetErrorObj();
  int query_status=MS_NOERR;
  int query_layer = 0;
  const char *format_list=NULL;
  int valid_format=MS_FALSE;
  int format_found = MS_FALSE;
  int use_bbox = MS_FALSE;
  int wms_layer =  MS_FALSE;
  const char *wms_connection = NULL;
  int numOWSLayers = 0;
  char ***nestedGroups = NULL;
  int *numNestedGroups = NULL;
  int *isUsedInNestedGroup = NULL;

  nestedGroups = (char***)msSmallCalloc(map->numlayers, sizeof(char**));
  numNestedGroups = (int*)msSmallCalloc(map->numlayers, sizeof(int));
  isUsedInNestedGroup = (int*)msSmallCalloc(map->numlayers, sizeof(int));
  msWMSPrepareNestedGroups(map, nVersion, nestedGroups, numNestedGroups, isUsedInNestedGroup);

  for(i=0; i<numentries; i++) {
    if(strcasecmp(names[i], "QUERY_LAYERS") == 0) {
      char **layers;
      int numlayers, j, k;
      const char* rootlayer_name;

      query_layer = 1; /* flag set if QUERY_LAYERS is the request */

      layers = msStringSplit(values[i], ',', &numlayers);
      if(layers==NULL || numlayers < 1 || strlen(msStringTrimLeft(values[i])) < 1) {
        msSetError(MS_WMSERR, "At least one layer name required in QUERY_LAYERS.", "msWMSFeatureInfo()");
        return msWMSException(map, nVersion, "LayerNotDefined", wms_exception_format);
      }

      for(j=0; j<map->numlayers; j++) {
        /* Force all layers OFF by default */
        GET_LAYER(map, j)->status = MS_OFF;
      }

      /* Special case for root layer */
      rootlayer_name = msOWSLookupMetadata(&(map->web.metadata), "MO", "rootlayer_name");
      if( !rootlayer_name )
          rootlayer_name = map->name;
      if( rootlayer_name && msStringInArray(rootlayer_name, layers, numlayers) ) {
        for(j=0; j<map->numlayers; j++) {
          layerObj* layer = GET_LAYER(map, j);
          if( msIsLayerQueryable(layer) &&
              msIntegerInArray(layer->index, ows_request->enabled_layers, ows_request->numlayers) ) {
            if (layer->connectiontype == MS_WMS) {
              wms_layer = MS_TRUE;
              wms_connection = layer->connection;
            }

            numlayers_found++;
            layer->status = MS_ON;
          }
        }
      }

      for(j=0; j<map->numlayers; j++) {
        layerObj* layer = GET_LAYER(map, j);
        if( !msIsLayerQueryable(layer) )
            continue;
        for(k=0; k<numlayers; k++) {
          if (((layer->name && strcasecmp(layer->name, layers[k]) == 0) ||
               (layer->group && strcasecmp(layer->group, layers[k]) == 0) ||
               ((numNestedGroups[j] >0) && msStringInArray(layers[k], nestedGroups[j], numNestedGroups[j]))) &&
              (msIntegerInArray(layer->index, ows_request->enabled_layers, ows_request->numlayers)) ) {

            if (layer->connectiontype == MS_WMS) {
              wms_layer = MS_TRUE;
              wms_connection = layer->connection;
            }

            numlayers_found++;
            layer->status = MS_ON;
          }
        }
      }

      msFreeCharArray(layers, numlayers);
    } else if (strcasecmp(names[i], "INFO_FORMAT") == 0) {
      if (values[i] && strlen(values[i]) > 0) {
        info_format = values[i];
        format_found = MS_TRUE;
      }
    } else if (strcasecmp(names[i], "FEATURE_COUNT") == 0)
      feature_count = atoi(values[i]);
    else if(strcasecmp(names[i], "X") == 0 || strcasecmp(names[i], "I") == 0)
      point.x = atof(values[i]);
    else if (strcasecmp(names[i], "Y") == 0 || strcasecmp(names[i], "J") == 0)
      point.y = atof(values[i]);
    else if (strcasecmp(names[i], "RADIUS") == 0) {
      /* RADIUS in pixels. */
      /* This is not part of the spec, but some servers such as cubeserv */
      /* support it as a vendor-specific feature. */
      /* It's easy for MapServer to handle this so let's do it! */

      /* Special RADIUS value that changes the query into a bbox query */
      /* based on the bbox in the request parameters. */
      if( strcasecmp(values[i], "BBOX") == 0) {
        use_bbox = MS_TRUE;
      } else {
        int j;
        for(j=0; j<map->numlayers; j++) {
          GET_LAYER(map, j)->tolerance = atoi(values[i]);
          GET_LAYER(map, j)->toleranceunits = MS_PIXELS;
        }
      }
    }

  }

  /* free the stuff used for nested layers */
  for (i = 0; i < map->numlayers; i++) {
    if (numNestedGroups[i] > 0) {
      msFreeCharArray(nestedGroups[i], numNestedGroups[i]);
    }
  }
  free(nestedGroups);
  free(numNestedGroups);
  free(isUsedInNestedGroup);

  if(numlayers_found == 0) {
    if (query_layer) {
      msSetError(MS_WMSERR, "Layer(s) specified in QUERY_LAYERS parameter is not offered by the service instance.", "msWMSFeatureInfo()");
      return msWMSException(map, nVersion, "LayerNotDefined", wms_exception_format);
    } else {
      msSetError(MS_WMSERR, "Required QUERY_LAYERS parameter missing for getFeatureInfo.", "msWMSFeatureInfo()");
      return msWMSException(map, nVersion, "LayerNotDefined", wms_exception_format);
    }
  }

  /*make sure to initialize the map scale so that layers that are scale dependent are resepected for
    the query*/
  msCalculateScale(map->extent,map->units,map->width,map->height, map->resolution, &map->scaledenom);

  /* -------------------------------------------------------------------- */
  /*      check if all layers selected are queryable. If not send an      */
  /*      exception.                                                      */
  /* -------------------------------------------------------------------- */

  /* If a layer of type WMS was found... all layers have to be of that type and with the same connection */
  for (i=0; i<map->numlayers; i++) {
    if (GET_LAYER(map, i)->status == MS_ON) {
      if (wms_layer == MS_TRUE) {
        if ( (GET_LAYER(map, i)->connectiontype != MS_WMS) || (strcasecmp(wms_connection, GET_LAYER(map, i)->connection) != 0) ) {
          msSetError(MS_WMSERR, "Requested WMS layer(s) are not queryable: type or connection differ", "msWMSFeatureInfo()");
          return msWMSException(map, nVersion, "LayerNotQueryable", wms_exception_format);
        }
        ++numOWSLayers;
      }
    }
  }

  /* It's a valid Cascading WMS GetFeatureInfo request */
  if (wms_layer)
    return msWMSLayerExecuteRequest(map, numOWSLayers, point.x, point.y,
                                    feature_count, info_format, WMS_GETFEATUREINFO);

  if( use_bbox == MS_FALSE ) {

    if(point.x == -1.0 || point.y == -1.0) {
      if (nVersion >= OWS_1_3_0)
        msSetError(MS_WMSERR, "Required I/J parameters missing for getFeatureInfo.", "msWMSFeatureInfo()");
      else
        msSetError(MS_WMSERR, "Required X/Y parameters missing for getFeatureInfo.", "msWMSFeatureInfo()");
      return msWMSException(map, nVersion, NULL, wms_exception_format);
    }

    /*wms1.3.0: check if the points are valid*/
    if (nVersion >= OWS_1_3_0) {
      if (point.x > map->width || point.y > map->height) {
        msSetError(MS_WMSERR, "Invalid I/J values", "msWMSFeatureInfo()");
        return msWMSException(map, nVersion, "InvalidPoint", wms_exception_format);
      }
    }
    /* Perform the actual query */
    cellx = MS_CELLSIZE(map->extent.minx, map->extent.maxx, map->width); /* note: don't adjust extent, WMS assumes incoming extent is correct */
    celly = MS_CELLSIZE(map->extent.miny, map->extent.maxy, map->height);
    point.x = MS_IMAGE2MAP_X(point.x, map->extent.minx, cellx);
    point.y = MS_IMAGE2MAP_Y(point.y, map->extent.maxy, celly);

    /* WMS 1.3.0 states that feature_count is *per layer*.
     * Its value is a positive integer, if omitted then the default is 1
     */
    if (feature_count < 1)
      feature_count = 1;

    map->query.type = MS_QUERY_BY_POINT;
    map->query.mode = (feature_count==1?MS_QUERY_SINGLE:MS_QUERY_MULTIPLE);
    map->query.layer = -1;
    map->query.point = point;
    map->query.buffer = 0;
    map->query.maxresults = feature_count;

    if(msQueryByPoint(map) != MS_SUCCESS)
      if((query_status=ms_error->code) != MS_NOTFOUND) return msWMSException(map, nVersion, NULL, wms_exception_format);

  } else { /* use_bbox == MS_TRUE */
    map->query.type = MS_QUERY_BY_RECT;
    map->query.mode = MS_QUERY_MULTIPLE;
    map->query.layer = -1;
    map->query.rect = map->extent;
    map->query.buffer = 0;
    map->query.maxresults = feature_count;
    if(msQueryByRect(map) != MS_SUCCESS)
      if((query_status=ms_error->code) != MS_NOTFOUND) return msWMSException(map, nVersion, NULL, wms_exception_format);
  }

  /*validate the INFO_FORMAT*/
  valid_format = MS_FALSE;
  format_list = msOWSLookupMetadata(&(map->web.metadata), "M",
                                    "getfeatureinfo_formatlist");
  /*feature_info_mime_type depricated for MapServer 6.0*/
  if (!format_list)
    format_list = msOWSLookupMetadata(&(map->web.metadata), "MO",
                                      "feature_info_mime_type");
  if (format_list) {
    /*can not really validate if it is a valid output format
      since old way of using template with web->header/footer and
      layer templates need to still be supported.
      We can only validate if it was part of the format list*/
    if (strcasestr(format_list, info_format))
      valid_format = MS_TRUE;
  }
  /*check to see if the format passed is text/plain or GML and if is
    defined in the formatlist. If that is the case, It is a valid format*/
  if (strcasecmp(info_format, "MIME") == 0 ||
      strcasecmp(info_format, "text/plain") == 0 ||
      strncasecmp(info_format, "GML", 3) == 0 ||
      strcasecmp(info_format, "application/vnd.ogc.gml") == 0)
    valid_format = MS_TRUE;


  /*last case: if the info_format is not part of the request, it defaults to MIME*/
  if (!valid_format && format_found == MS_FALSE)
    valid_format =MS_TRUE;

  if (!valid_format) {
    msSetError(MS_WMSERR, "Unsupported INFO_FORMAT value (%s).",
               "msWMSFeatureInfo()", info_format);
    if (nVersion >= OWS_1_3_0)
      return msWMSException(map, nVersion, "InvalidFormat", wms_exception_format);
    else
      return msWMSException(map, nVersion, NULL, wms_exception_format);
  }

  /* Generate response */
  if (strcasecmp(info_format, "MIME") == 0 ||
      strcasecmp(info_format, "text/plain") == 0) {

    /* MIME response... we're free to use any valid MIME type */
    int numresults = 0;

    msIO_setHeader("Content-Type","text/plain; charset=UTF-8");
    msIO_sendHeaders();
    msIO_printf("GetFeatureInfo results:\n");

    numresults = msDumpResult(map, 0, nVersion, wms_exception_format);

    if (numresults == 0) msIO_printf("\n  Search returned no results.\n");

  } else if (strncasecmp(info_format, "GML", 3) == 0 ||  /* accept GML.1 or GML */
             strcasecmp(info_format, "application/vnd.ogc.gml") == 0) {

    if (nVersion <= OWS_1_0_7)   /* 1.0.0 to 1.0.7 */
      msIO_setHeader("Content-Type","text/xml; charset=UTF-8");
    else /* 1.1.0 and later */
      msIO_setHeader("Content-Type","application/vnd.ogc.gml; charset=UTF-8");
    msIO_sendHeaders();
    msGMLWriteQuery(map, NULL, "MGO"); /* default is stdout */

  } else {
    mapservObj *msObj;

    char **translated_names, **translated_values;
    int translated_numentries;
    msObj = msAllocMapServObj();

    /* Translate some vars from WMS to mapserv */
    msTranslateWMS2Mapserv((const char**)names, (const char**)values, numentries, &translated_names, &translated_values, &translated_numentries);

    msObj->map = map;
    msFreeCharArray(msObj->request->ParamNames, msObj->request->NumParams);
    msFreeCharArray(msObj->request->ParamValues, msObj->request->NumParams);
    msObj->request->ParamNames = translated_names;
    msObj->request->ParamValues = translated_values;
    msObj->Mode = QUERY;
    msObj->request->NumParams = translated_numentries;
    msObj->mappnt.x = point.x;
    msObj->mappnt.y = point.y;

    if (query_status == MS_NOTFOUND && msObj->map->web.empty) {
      if(msReturnURL(msObj, msObj->map->web.empty, BROWSE) != MS_SUCCESS)
        return msWMSException(map, nVersion, NULL, wms_exception_format);
    } else if (msReturnTemplateQuery(msObj, (char *)info_format, NULL) != MS_SUCCESS)
      return msWMSException(map, nVersion, NULL, wms_exception_format);

    /* We don't want to free the map since it */
    /* belongs to the caller, set it to NULL before freeing the mapservObj */
    msObj->map = NULL;

    msFreeMapServObj(msObj);
  }

  return(MS_SUCCESS);
}

/*
** msWMSDescribeLayer()
*/
int msWMSDescribeLayer(mapObj *map, int nVersion, char **names,
                       char **values, int numentries, char *wms_exception_format)
{
  int i = 0;
  char **layers = NULL;
  int numlayers = 0;
  int j, k;
  layerObj *lp = NULL;
  const char *pszOnlineResMapWFS = NULL, *pszOnlineResLyrWFS = NULL;
  const char *pszOnlineResMapWCS = NULL, *pszOnlineResLyrWCS = NULL;
  char *pszOnlineResEncoded=NULL, *pszLayerName=NULL;
  char *schemalocation = NULL;
  char *version = NULL;
  char *sld_version = NULL;
  char ***nestedGroups = NULL;
  int *numNestedGroups = NULL;
  int *isUsedInNestedGroup = NULL;

  for(i=0; map && i<numentries; i++) {
    if(strcasecmp(names[i], "LAYERS") == 0) {
      if(layers) msFreeCharArray(layers, numlayers); /* free previous occurence of layers array if more than one was provided */
      layers = msStringSplit(values[i], ',', &numlayers);
    }
    if(strcasecmp(names[i], "VERSION") == 0) {
      version = values[i];
    }
    if(strcasecmp(names[i], "SLD_VERSION") == 0) {
      sld_version = values[i];
    }
  }

  if (nVersion >= OWS_1_3_0 && sld_version == NULL) {
    msSetError(MS_WMSERR, "Missing required parameter SLD_VERSION", "DescribeLayer()");
    return msWMSException(map, nVersion, "MissingParameterValue", wms_exception_format);
  }
  if (nVersion >= OWS_1_3_0 && strcasecmp(sld_version, "1.1.0") != 0) {
    msSetError(MS_WMSERR, "SLD_VERSION must be 1.1.0", "DescribeLayer()");
    return msWMSException(map, nVersion, "InvalidParameterValue", wms_exception_format);
  }

  msIO_setHeader("Content-Type","text/xml; charset=UTF-8");
  msIO_sendHeaders();

  msIO_printf("<?xml version='1.0' encoding=\"UTF-8\"?>\n");

  schemalocation = msEncodeHTMLEntities(msOWSGetSchemasLocation(map));
  if (nVersion < OWS_1_3_0) {

    msIO_printf("<!DOCTYPE WMS_DescribeLayerResponse SYSTEM \"%s/wms/1.1.1/WMS_DescribeLayerResponse.dtd\">\n", schemalocation);


    msIO_printf("<WMS_DescribeLayerResponse version=\"%s\" >\n", version);
  } else {
    msIO_printf("<DescribeLayerResponse xmlns=\"http://www.opengis.net/sld\" xmlns:ows=\"http://www.opengis.net/ows\" xmlns:se=\"http://www.opengis.net/se\" xmlns:wfs=\"http://www.opengis.net/wfs\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xsi:schemaLocation=\"http://www.opengis.net/sld %s/sld/1.1.0/DescribeLayer.xsd\">\n", schemalocation);
    msIO_printf("<Version>%s</Version>\n",sld_version);

  }
  free(schemalocation);

  /* check if map-level metadata wfs(wcs)_onlineresource is available */
  pszOnlineResMapWFS = msOWSLookupMetadata(&(map->web.metadata), "FO", "onlineresource");
  if (pszOnlineResMapWFS && strlen(pszOnlineResMapWFS) == 0)
    pszOnlineResMapWFS = NULL;

  pszOnlineResMapWCS = msOWSLookupMetadata(&(map->web.metadata), "CO", "onlineresource");
  if (pszOnlineResMapWCS && strlen(pszOnlineResMapWCS) == 0)
    pszOnlineResMapWCS = NULL;

  nestedGroups = (char***)msSmallCalloc(map->numlayers, sizeof(char**));
  numNestedGroups = (int*)msSmallCalloc(map->numlayers, sizeof(int));
  isUsedInNestedGroup = (int*)msSmallCalloc(map->numlayers, sizeof(int));
  msWMSPrepareNestedGroups(map, nVersion, nestedGroups, numNestedGroups, isUsedInNestedGroup);

  for(j=0; j<numlayers; j++) {
    for(k=0; k<map->numlayers; k++) {
      lp = GET_LAYER(map, k);

      if ((map->name && strcasecmp(map->name, layers[j]) == 0) ||
          (lp->name && strcasecmp(lp->name, layers[j]) == 0) ||
          (lp->group && strcasecmp(lp->group, layers[j]) == 0) ||
          ((numNestedGroups[k] >0) && msStringInArray(layers[j], nestedGroups[k], numNestedGroups[k]))) {
        /* Look for a WFS onlineresouce at the layer level and then at
         * the map level.
         */
        pszOnlineResLyrWFS = msOWSLookupMetadata(&(lp->metadata), "FO",
                             "onlineresource");
        pszOnlineResLyrWCS = msOWSLookupMetadata(&(lp->metadata), "CO",
                             "onlineresource");
        if (pszOnlineResLyrWFS == NULL || strlen(pszOnlineResLyrWFS) == 0)
          pszOnlineResLyrWFS = pszOnlineResMapWFS;

        if (pszOnlineResLyrWCS == NULL || strlen(pszOnlineResLyrWCS) == 0)
          pszOnlineResLyrWCS = pszOnlineResMapWCS;

        if (pszOnlineResLyrWFS && (lp->type == MS_LAYER_POINT ||
                                   lp->type == MS_LAYER_LINE ||
                                   lp->type == MS_LAYER_POLYGON) ) {
          pszOnlineResEncoded = msEncodeHTMLEntities(pszOnlineResLyrWFS);
          pszLayerName = msEncodeHTMLEntities(lp->name);

          if (nVersion < OWS_1_3_0) {
            msIO_printf("<LayerDescription name=\"%s\" wfs=\"%s\" owsType=\"WFS\" owsURL=\"%s\">\n",
                        pszLayerName, pszOnlineResEncoded, pszOnlineResEncoded);
            msIO_printf("<Query typeName=\"%s\" />\n", pszLayerName);
            msIO_printf("</LayerDescription>\n");
          } else { /*wms 1.3.0*/
            msIO_printf("  <LayerDescription>\n");
            msIO_printf("    <owsType>wfs</owsType>\n");
            msIO_printf("    <se:OnlineResource xlink:type=\"simple\" xlink:href=\"%s\"/>\n",
                        pszOnlineResEncoded);
            msIO_printf("    <TypeName>\n");
            msIO_printf("      <se:FeatureTypeName>%s</se:FeatureTypeName>\n",pszLayerName);
            msIO_printf("    </TypeName>\n");
            msIO_printf("  </LayerDescription>\n");
          }

          msFree(pszOnlineResEncoded);
          msFree(pszLayerName);
        } else if (pszOnlineResLyrWCS && lp->type == MS_LAYER_RASTER &&
                   lp->connectiontype != MS_WMS) {
          pszOnlineResEncoded = msEncodeHTMLEntities(pszOnlineResLyrWCS);
          pszLayerName = msEncodeHTMLEntities(lp->name);

          if (nVersion < OWS_1_3_0) {
            msIO_printf("<LayerDescription name=\"%s\"  owsType=\"WCS\" owsURL=\"%s\">\n",
                        pszLayerName, pszOnlineResEncoded);
            msIO_printf("<Query typeName=\"%s\" />\n", pszLayerName);
            msIO_printf("</LayerDescription>\n");

            msFree(pszOnlineResEncoded);
            msFree(pszLayerName);
          } else {
            msIO_printf("  <LayerDescription>\n");
            msIO_printf("    <owsType>wcs</owsType>\n");
            msIO_printf("    <se:OnlineResource xlink:type=\"simple\" xlink:href=\"%s\"/>\n",
                        pszOnlineResEncoded);
            msIO_printf("    <TypeName>\n");
            msIO_printf("      <se:CoverageTypeName>%s</se:CoverageTypeName>\n",pszLayerName);
            msIO_printf("    </TypeName>\n");
            msIO_printf("  </LayerDescription>\n");
          }
        } else {
          char *pszLayerName;
          pszLayerName = msEncodeHTMLEntities(lp->name);

          if (nVersion < OWS_1_3_0)
            msIO_printf("<LayerDescription name=\"%s\"></LayerDescription>\n",
                        pszLayerName);
          else { /*wms 1.3.0*/
            msIO_printf("  <LayerDescription>\n");
            /*need to have a owstype for the DescribeLayer to be valid*/
            if (lp->type == MS_LAYER_RASTER && lp->connectiontype != MS_WMS)
              msIO_printf("    <owsType>wcs</owsType>\n");
            else
              msIO_printf("    <owsType>wfs</owsType>\n");

            msIO_printf("    <se:OnlineResource xlink:type=\"simple\"/>\n");
            msIO_printf("    <TypeName>\n");
            if (lp->type == MS_LAYER_RASTER && lp->connectiontype != MS_WMS)
              msIO_printf("      <se:CoverageTypeName>%s</se:CoverageTypeName>\n",pszLayerName);
            else
              msIO_printf("      <se:FeatureTypeName>%s</se:FeatureTypeName>\n",pszLayerName);
            msIO_printf("    </TypeName>\n");
            msIO_printf("  </LayerDescription>\n");
          }

          msFree(pszLayerName);
        }
        /* break; */
      }
    }
  }

  if (nVersion < OWS_1_3_0)
    msIO_printf("</WMS_DescribeLayerResponse>\n");
  else
    msIO_printf("</DescribeLayerResponse>\n");

  if (layers)
    msFreeCharArray(layers, numlayers);

  /* free the stuff used for nested layers */
  for (i = 0; i < map->numlayers; i++) {
    if (numNestedGroups[i] > 0) {
      msFreeCharArray(nestedGroups[i], numNestedGroups[i]);
    }
  }
  free(nestedGroups);
  free(numNestedGroups);
  free(isUsedInNestedGroup);

  return(MS_SUCCESS);
}


/*
** msWMSGetLegendGraphic()
*/
int msWMSLegendGraphic(mapObj *map, int nVersion, char **names,
                       char **values, int numentries, char *wms_exception_format, owsRequestObj *ows_request,
                       map_hittest *hittest)
{
  char *pszLayer = NULL;
  char *pszFormat = NULL;
  char *psRule = NULL;
  char *psScale = NULL;
  int iLayerIndex = -1;
  outputFormatObj *psFormat = NULL;
  imageObj *img=NULL;
  int j, i = 0;
  int nWidth = -1, nHeight =-1;
  char *pszStyle = NULL;
  char *sld_version = NULL;
  int wms_layer =  MS_FALSE;
  const char *sldenabled = NULL;
  const char *format_list = NULL;
  layerObj *lp;
  int nLayers =0;
  char ***nestedGroups = NULL;
  int *numNestedGroups = NULL;
  int *isUsedInNestedGroup = NULL;

  if(!hittest) {
    /* we can skip alot of testing if we already have a hittest, as it has already been done in the hittesting phase */


    sldenabled = msOWSLookupMetadata(&(map->web.metadata), "MO", "sld_enabled");

    if (sldenabled == NULL)
      sldenabled = "true";

    for(i=0; map && i<numentries; i++) {
      if (strcasecmp(names[i], "LAYER") == 0) {
        pszLayer = values[i];
      } else if (strcasecmp(names[i], "WIDTH") == 0)
        nWidth = atoi(values[i]);
      else if (strcasecmp(names[i], "HEIGHT") == 0)
        nHeight = atoi(values[i]);
      else if (strcasecmp(names[i], "FORMAT") == 0)
        pszFormat = values[i];
      else if (strcasecmp(names[i], "SCALE") == 0)
        psScale = values[i];

      /* -------------------------------------------------------------------- */
      /*      SLD support :                                                   */
      /*        - check if the SLD parameter is there. it is supposed to      */
      /*      refer a valid URL containing an SLD document.                   */
      /*        - check the SLD_BODY parameter that should contain the SLD    */
      /*      xml string.                                                     */
      /* -------------------------------------------------------------------- */
      else if (strcasecmp(names[i], "SLD") == 0 &&
          values[i] && strlen(values[i]) > 0 && strcasecmp(sldenabled, "true") == 0)
        msSLDApplySLDURL(map, values[i], -1, NULL, NULL);
      else if (strcasecmp(names[i], "SLD_BODY") == 0 &&
          values[i] && strlen(values[i]) > 0 && strcasecmp(sldenabled, "true") == 0)
        msSLDApplySLD(map, values[i], -1, NULL, NULL);
      else if (strcasecmp(names[i], "RULE") == 0)
        psRule = values[i];
      else if (strcasecmp(names[i], "STYLE") == 0)
        pszStyle = values[i];

      /* -------------------------------------------------------------------- */
      /*      SLD support:                                                    */
      /*        - because the request parameter "sld_version" is required in  */
      /*          in WMS 1.3.0, it will be set regardless of OGR support.     */
      /* -------------------------------------------------------------------- */
      else if(strcasecmp(names[i], "SLD_VERSION") == 0)
        sld_version = values[i];
    }

    if (!pszLayer) {
      msSetError(MS_WMSERR, "Mandatory LAYER parameter missing in GetLegendGraphic request.", "msWMSGetLegendGraphic()");
      return msWMSException(map, nVersion, "LayerNotDefined", wms_exception_format);
    }
    if (!pszFormat) {
      msSetError(MS_WMSERR, "Mandatory FORMAT parameter missing in GetLegendGraphic request.", "msWMSGetLegendGraphic()");
      return msWMSException(map, nVersion, "InvalidFormat", wms_exception_format);
    }

    if (nVersion >= OWS_1_3_0 && sld_version == NULL) {
      msSetError(MS_WMSERR, "Missing required parameter SLD_VERSION", "GetLegendGraphic()");
      return msWMSException(map, nVersion, "MissingParameterValue", wms_exception_format);
    }
    if (nVersion >= OWS_1_3_0 && strcasecmp(sld_version, "1.1.0") != 0) {
      msSetError(MS_WMSERR, "SLD_VERSION must be 1.1.0", "GetLegendGraphic()");
      return msWMSException(map, nVersion, "InvalidParameterValue", wms_exception_format);
    }

    nestedGroups = (char***)msSmallCalloc(map->numlayers, sizeof(char**));
    numNestedGroups = (int*)msSmallCalloc(map->numlayers, sizeof(int));
    isUsedInNestedGroup = (int*)msSmallCalloc(map->numlayers, sizeof(int));
    msWMSPrepareNestedGroups(map, nVersion, nestedGroups, numNestedGroups, isUsedInNestedGroup);

    /* check if layer name is valid. we check for layer's and group's name */
    /* as well as wms_layer_group names */
    for (i=0; i<map->numlayers; i++) {
      lp = GET_LAYER(map, i);
      if (  ((map->name && strcasecmp(map->name, pszLayer) == 0) ||
          (lp->name && strcasecmp(lp->name, pszLayer) == 0) ||
          (lp->group && strcasecmp(lp->group, pszLayer) == 0) ||
          ((numNestedGroups[i] >0) && (msStringInArray(pszLayer, nestedGroups[i], numNestedGroups[i]))) ) &&
          (msIntegerInArray(lp->index, ows_request->enabled_layers, ows_request->numlayers)) ) {
        nLayers++;
        lp->status = MS_ON;
        iLayerIndex = i;
        if (GET_LAYER(map, i)->connectiontype == MS_WMS) {
          /* we do not cascade a wms layer if it contains at least
           * one class with the property name set */
          wms_layer = MS_TRUE;
          for (j=0; j<lp->numclasses; j++) {
            if (lp->class[j]->name != NULL && strlen(lp->class[j]->name)>0) {
              wms_layer = MS_FALSE;
              break;
            }
          }
        }
      } else
        lp->status = MS_OFF;
    }

    /* free the stuff used for nested layers */
    for (i = 0; i < map->numlayers; i++) {
      if (numNestedGroups[i] > 0) {
        msFreeCharArray(nestedGroups[i], numNestedGroups[i]);
      }
    }
    free(nestedGroups);
    free(numNestedGroups);
    free(isUsedInNestedGroup);

    if (nLayers == 0) {
      msSetError(MS_WMSERR, "Invalid layer given in the LAYER parameter. A layer might be disabled for \
this request. Check wms/ows_enable_request settings.",
"msWMSGetLegendGraphic()");
      return msWMSException(map, nVersion, "LayerNotDefined", wms_exception_format);
    }


    /* if SCALE was provided in request, calculate an extent and use a default width and height */
    if ( psScale != NULL ) {
      double scale, cellsize;

      scale = atof(psScale);
      map->width = 600;
      map->height = 600;

      cellsize = (scale/map->resolution)/msInchesPerUnit(map->units, 0.0);

      map->extent.maxx = cellsize*map->width/2.0;
      map->extent.maxy = cellsize*map->height/2.0;
      map->extent.minx = -map->extent.maxx;
      map->extent.miny = -map->extent.maxy;
    }

    /* It's a valid Cascading WMS GetLegendGraphic request */
    if (wms_layer)
      return msWMSLayerExecuteRequest(map, 1, 0, 0,
          0, NULL, WMS_GETLEGENDGRAPHIC);

    /*if STYLE is set, check if it is a valid style (valid = at least one
    of the classes have a the group value equals to the style */
    /*style is only validated when there is only one layer #3411*/
    if (nLayers == 1 &&  pszStyle && strlen(pszStyle) > 0 && strcasecmp(pszStyle, "default") != 0) {
      for (i=0; i<GET_LAYER(map, iLayerIndex)->numclasses; i++) {
        if (GET_LAYER(map, iLayerIndex)->class[i]->group &&
            strcasecmp(GET_LAYER(map, iLayerIndex)->class[i]->group, pszStyle) == 0)
          break;
      }

      if (i == GET_LAYER(map, iLayerIndex)->numclasses) {
        msSetError(MS_WMSERR, "style used in the STYLE parameter is not defined on the layer.",
            "msWMSGetLegendGraphic()");
        return msWMSException(map, nVersion, "StyleNotDefined", wms_exception_format);
      } else {
        msFree(GET_LAYER(map, iLayerIndex)->classgroup);
        GET_LAYER(map, iLayerIndex)->classgroup = msStrdup(pszStyle);

      }
    }
  } else {
    /* extract the parameters we need */
    for(i=0; map && i<numentries; i++) {
      if (strcasecmp(names[i], "FORMAT") == 0)
        pszFormat = values[i];
      else if (strcasecmp(names[i], "RULE") == 0)
        psRule = values[i];
    }
  }
  /* validate format */

  /*check to see if a predefined list is given*/
  format_list = msOWSLookupMetadata(&(map->web.metadata), "M","getlegendgraphic_formatlist");
  if (format_list) {
    psFormat = msOwsIsOutputFormatValid(map, pszFormat, &(map->web.metadata),
        "M", "getlegendgraphic_formatlist");
    if (psFormat == NULL) {
      msSetError(MS_IMGERR,
          "Unsupported output format (%s).",
          "msWMSGetLegendGraphic()",
          pszFormat);
      return msWMSException(map, nVersion, "InvalidFormat", wms_exception_format);
    }
  } else {
    psFormat = msSelectOutputFormat( map, pszFormat);
    if( psFormat == NULL || ! MS_RENDERER_PLUGIN(psFormat) )
      /* msDrawLegend and msCreateLegendIcon both switch the alpha channel to gd
       ** after creation, so they can be called here without going through
       ** the msAlphaGD2AGG functions */
    {
      msSetError(MS_IMGERR,
          "Unsupported output format (%s).",
          "msWMSGetLegendGraphic()",
          pszFormat);
      return msWMSException(map, nVersion, "InvalidFormat", wms_exception_format);
    }
  }
  msApplyOutputFormat(&(map->outputformat), psFormat, MS_NOOVERRIDE,
      MS_NOOVERRIDE, MS_NOOVERRIDE );
  
  if ( psRule == NULL || nLayers > 1) {
    if ( psScale != NULL ) {
      /* Scale-dependent legend. map->scaledenom will be calculated in msDrawLegend */
      img = msDrawLegend(map, MS_FALSE, NULL);
    } else {
      /* Scale-independent legend */
      img = msDrawLegend(map, MS_TRUE, hittest);
    }
  } else {
    /* RULE was specified. Get the class corresponding to the RULE */
    /* (RULE = class->name) */
    /* TBT FIXME? also check the map->scaledenom if multiple scale-dependant classes with same name */

    for (i=0; i<GET_LAYER(map, iLayerIndex)->numclasses; i++) {
      if (GET_LAYER(map, iLayerIndex)->classgroup &&
          (GET_LAYER(map, iLayerIndex)->class[i]->group == NULL ||
           strcasecmp(GET_LAYER(map, iLayerIndex)->class[i]->group,
                      GET_LAYER(map, iLayerIndex)->classgroup) != 0))
        continue;

      if (GET_LAYER(map, iLayerIndex)->class[i]->name &&
          strlen(GET_LAYER(map, iLayerIndex)->class[i]->name) > 0 &&
          strcasecmp(GET_LAYER(map, iLayerIndex)->class[i]->name,psRule) == 0)
        break;

    }
    if (i < GET_LAYER(map, iLayerIndex)->numclasses) {
      /* set the map legend parameters */
      if (nWidth < 0) {
        if (map->legend.keysizex > 0)
          nWidth = map->legend.keysizex;
        else
          nWidth = 20; /* default values : this in not defined in the specs */
      }
      if (nHeight < 0) {
        if (map->legend.keysizey > 0)
          nHeight = map->legend.keysizey;
        else
          nHeight = 20;
      }

      if ( psScale != NULL ) {
        /* Scale-dependent legend. calculate map->scaledenom */
        map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);
        msCalculateScale(map->extent, map->units, map->width, map->height, map->resolution, &map->scaledenom);
        img = msCreateLegendIcon(map, GET_LAYER(map, iLayerIndex),
                                 GET_LAYER(map, iLayerIndex)->class[i],
                                 nWidth, nHeight, MS_FALSE);
      } else {
        /* Scale-independent legend */
        img = msCreateLegendIcon(map, GET_LAYER(map, iLayerIndex),
                                 GET_LAYER(map, iLayerIndex)->class[i],
                                 nWidth, nHeight, MS_TRUE);
      }
    }
    if (img == NULL) {
      msSetError(MS_IMGERR,
                 "Unavailable RULE (%s).",
                 "msWMSGetLegendGraphic()",
                 psRule);
      return msWMSException(map, nVersion, "InvalidRule", wms_exception_format);
    }
  }

  if (img == NULL)
    return msWMSException(map, nVersion, NULL, wms_exception_format);

  msIO_setHeader("Content-Type", "%s", MS_IMAGE_MIME_TYPE(map->outputformat));
  msIO_sendHeaders();
  if (msSaveImage(map, img, NULL) != MS_SUCCESS)
    return msWMSException(map, nVersion, NULL, wms_exception_format);

  msFreeImage(img);

  return(MS_SUCCESS);
}

/*
** msWMSGetContentDependantLegend()
*/
int msWMSGetContentDependantLegend(mapObj *map, int nVersion, char **names, char **values, int numentries,
                char *wms_exception_format, owsRequestObj *ows_request) {

  int i,status;
  map_hittest hittest;
  /* turn off layer if WMS GetMap is not enabled */
  for (i=0; i<map->numlayers; i++)
    if (!msIntegerInArray(GET_LAYER(map, i)->index, ows_request->enabled_layers, ows_request->numlayers))
      GET_LAYER(map, i)->status = MS_OFF;

  initMapHitTests(map,&hittest);
  status = msHitTestMap(map,&hittest);
  if(status == MS_SUCCESS) {
    status = msWMSLegendGraphic(map,nVersion,names,values,numentries,wms_exception_format,ows_request,&hittest);
  }
  freeMapHitTests(map,&hittest);
  if(status != MS_SUCCESS) {
    return msWMSException(map, nVersion, NULL, wms_exception_format);
  } else {
    return MS_SUCCESS;
  }
}

/*
** msWMSGetStyles() : return an SLD document for all layers that
** have a status set to on or default.
*/
int msWMSGetStyles(mapObj *map, int nVersion, char **names,
                   char **values, int numentries, char *wms_exception_format)

{
  int i,j,k;
  int validlayer = 0;
  int numlayers = 0;
  char **layers = NULL;
  char  *sld = NULL;

  char ***nestedGroups = NULL;
  int *numNestedGroups = NULL;
  int *isUsedInNestedGroup = NULL;

  nestedGroups = (char***)msSmallCalloc(map->numlayers, sizeof(char**));
  numNestedGroups = (int*)msSmallCalloc(map->numlayers, sizeof(int));
  isUsedInNestedGroup = (int*)msSmallCalloc(map->numlayers, sizeof(int));
  msWMSPrepareNestedGroups(map, nVersion, nestedGroups, numNestedGroups, isUsedInNestedGroup);

  for(i=0; i<numentries; i++) {
    /* getMap parameters */
    if (strcasecmp(names[i], "LAYERS") == 0) {
      layers = msStringSplit(values[i], ',', &numlayers);
      if (layers==NULL || numlayers < 1) {
        msSetError(MS_WMSERR, "At least one layer name required in LAYERS.",
                   "msWMSGetStyles()");
        return msWMSException(map, nVersion, NULL, wms_exception_format);
      }
      for(j=0; j<map->numlayers; j++)
        GET_LAYER(map, j)->status = MS_OFF;

      for (k=0; k<numlayers; k++) {
        for (j=0; j<map->numlayers; j++) {
          if ((map->name &&
               strcasecmp(map->name, layers[k]) == 0) ||
              (GET_LAYER(map, j)->name &&
               strcasecmp(GET_LAYER(map, j)->name, layers[k]) == 0) ||
              (GET_LAYER(map, j)->group &&
               strcasecmp(GET_LAYER(map, j)->group, layers[k]) == 0) ||
              ((numNestedGroups[j] >0) && msStringInArray(layers[k], nestedGroups[j], numNestedGroups[j])) ) {
            GET_LAYER(map, j)->status = MS_ON;
            validlayer =1;
          }
        }
      }

      msFreeCharArray(layers, numlayers);
    }

  }

  /* free the stuff used for nested layers */
  for (i = 0; i < map->numlayers; i++) {
    if (numNestedGroups[i] > 0) {
      msFreeCharArray(nestedGroups[i], numNestedGroups[i]);
    }
  }
  free(nestedGroups);
  free(numNestedGroups);
  free(isUsedInNestedGroup);

  /* validate all layers given. If an invalid layer is sent, return an exception. */
  if (validlayer == 0) {
    msSetError(MS_WMSERR, "Invalid layer(s) given in the LAYERS parameter. A layer might be disabled for \
this request. Check wms/ows_enable_request settings.",
               "msWMSGetStyles()");
    return msWMSException(map, nVersion, "LayerNotDefined", wms_exception_format);
  }

  if (nVersion <= OWS_1_1_1) {
    msIO_setHeader("Content-Type","application/vnd.ogc.sld+xml; charset=UTF-8");
    msIO_sendHeaders();
    sld = msSLDGenerateSLD(map, -1, "1.0.0");
  } else {
    /*for wms 1.3.0 generate a 1.1 sld*/
    msIO_setHeader("Content-Type","text/xml; charset=UTF-8");
    msIO_sendHeaders();
    sld = msSLDGenerateSLD(map, -1, "1.1.0");
  }
  if (sld) {
    msIO_printf("%s\n", sld);
    free(sld);
  }

  return(MS_SUCCESS);
}

int msWMSGetSchemaExtension(mapObj *map)
{
  char *schemalocation = NULL;

  schemalocation = msEncodeHTMLEntities(msOWSGetSchemasLocation(map));

  msIO_setHeader("Content-Type","text/xml; charset=UTF-8");
  msIO_sendHeaders();

  msIO_printf("<?xml version='1.0' encoding=\"UTF-8\"?>\n");
  msIO_printf("<schema xmlns=\"http://www.w3.org/2001/XMLSchema\" xmlns:wms=\"http://www.opengis.net/wms\" xmlns:ms=\"http://mapserver.gis.umn.edu/mapserver\" targetNamespace=\"http://mapserver.gis.umn.edu/mapserver\" elementFormDefault=\"qualified\" version=\"1.0.0\">\n");
  msIO_printf("  <import namespace=\"http://www.opengis.net/wms\" schemaLocation=\"%s/wms/1.3.0/capabilities_1_3_0.xsd\"/>\n", schemalocation);
  msIO_printf("  <element name=\"GetStyles\" type=\"wms:OperationType\" substitutionGroup=\"wms:_ExtendedOperation\"/>\n");
  msIO_printf("</schema>");

  free(schemalocation);

  return(MS_SUCCESS);
}

#endif /* USE_WMS_SVR */


/*
** msWMSDispatch() is the entry point for WMS requests.
** - If this is a valid request then it is processed and MS_SUCCESS is returned
**   on success, or MS_FAILURE on failure.
** - If this does not appear to be a valid WMS request then MS_DONE
**   is returned and MapServer is expected to process this as a regular
**   MapServer request.
*/
int msWMSDispatch(mapObj *map, cgiRequestObj *req, owsRequestObj *ows_request, int force_wms_mode)
{
#ifdef USE_WMS_SVR
  int i, status, nVersion=OWS_VERSION_NOTSET, isContentDependantLegend = 0;
  const char *version=NULL, *request=NULL, *service=NULL, *format=NULL, *updatesequence=NULL, *language=NULL;
  char *wms_exception_format = NULL;
  char *validated_language = NULL;

  /*
  ** Process Params common to all requests
  */
  /* VERSION (WMTVER in 1.0.0) and REQUEST must be present in a valid request */
  for(i=0; i<req->NumParams; i++) {
    if(strcasecmp(req->ParamNames[i], "VERSION") == 0)
      version = req->ParamValues[i];
    else if (strcasecmp(req->ParamNames[i], "WMTVER") == 0 && version == NULL)
      version = req->ParamValues[i];
    else if (strcasecmp(req->ParamNames[i], "UPDATESEQUENCE") == 0)
      updatesequence = req->ParamValues[i];
    else if (strcasecmp(req->ParamNames[i], "REQUEST") == 0)
      request = req->ParamValues[i];
    else if (strcasecmp(req->ParamNames[i], "EXCEPTIONS") == 0)
      wms_exception_format = req->ParamValues[i];
    else if (strcasecmp(req->ParamNames[i], "SERVICE") == 0)
      service = req->ParamValues[i];
    else if (strcasecmp(req->ParamNames[i], "FORMAT") == 0)
      format = req->ParamValues[i];
    else if (strcasecmp(req->ParamNames[i], "LANGUAGE") == 0 && msOWSLookupMetadata(&(map->web.metadata), "MO", "inspire_capabilities"))
      language = req->ParamValues[i];
  }

  /* If SERVICE is specified then it MUST be "WMS" */
  if (service != NULL && strcasecmp(service, "WMS") != 0)
    return MS_DONE;  /* Not a WMS request */

  nVersion = msOWSParseVersionString(version);
  if (nVersion == OWS_VERSION_BADFORMAT) {
    /* Invalid version format. msSetError() has been called by
     * msOWSParseVersionString() and we return the error as an exception
     */
    return msWMSException(map, OWS_VERSION_NOTSET, NULL, wms_exception_format);
  }

  /*
  ** GetCapbilities request needs the service parametr defined as WMS:
  see section 7.1.3.2 wms 1.1.1 specs for decsription.
  */
  if (request && service == NULL &&
      (strcasecmp(request, "capabilities") == 0 ||
       strcasecmp(request, "GetCapabilities") == 0) &&
      (nVersion >= OWS_1_0_7 || nVersion == OWS_VERSION_NOTSET)) {
    if (force_wms_mode) {
      msSetError(MS_WMSERR, "Required SERVICE parameter missing.", "msWMSDispatch");
      return msWMSException(map, nVersion, "ServiceNotDefined", wms_exception_format);
    } else
      return MS_DONE;
  }

  /*
  ** Dispatch request... we should probably do some validation on VERSION here
  ** vs the versions we actually support.
  */
  if (request && (strcasecmp(request, "capabilities") == 0 ||
                  strcasecmp(request, "GetCapabilities") == 0) ) {
    const char *enable_request;
    int globally_enabled, disabled = MS_FALSE;

    if (nVersion == OWS_VERSION_NOTSET) {
      version = msOWSLookupMetadata(&(map->web.metadata), "M", "getcapabilities_version");
      if (version)
        nVersion = msOWSParseVersionString(version);
      else
        nVersion = OWS_1_3_0;/* VERSION is optional with getCapabilities only */
    }

    if ((status = msOWSMakeAllLayersUnique(map)) != MS_SUCCESS)
      return msWMSException(map, nVersion, NULL, wms_exception_format);

    msOWSRequestLayersEnabled(map, "M", "GetCapabilities", ows_request);

    enable_request = msOWSLookupMetadata(&map->web.metadata, "OM", "enable_request");
    globally_enabled = msOWSParseRequestMetadata(enable_request, "GetCapabilities", &disabled);

    if (ows_request->numlayers == 0 && !globally_enabled) {
      msSetError(MS_WMSERR, "WMS request not enabled. Check wms/ows_enable_request settings.", "msWMSGetCapabilities()");
      return msWMSException(map, nVersion, NULL, wms_exception_format);
    }
    msAcquireLock(TLOCK_WxS);
    status=msWMSGetCapabilities(map, nVersion, req, ows_request, updatesequence, wms_exception_format, language);
    msReleaseLock(TLOCK_WxS);
    return status;
  } else if (request && (strcasecmp(request, "context") == 0 ||
                         strcasecmp(request, "GetContext") == 0) ) {
    /* Return a context document with all layers in this mapfile
     * This is not a standard WMS request.
     * __TODO__ The real implementation should actually return only context
     * info for selected layers in the LAYERS parameter.
     */
    const char *getcontext_enabled;
    getcontext_enabled = msOWSLookupMetadata(&(map->web.metadata),
                         "MO", "getcontext_enabled");

    if (nVersion != OWS_VERSION_NOTSET) {
      /* VERSION, if specified, is Map Context version, not WMS version */
      /* Pass it via wms_context_version metadata */
      char szVersion[OWS_VERSION_MAXLEN];
      msInsertHashTable(&(map->web.metadata), "wms_context_version",
                        msOWSGetVersionString(nVersion, szVersion));
    }
    /* Now set version to 1.1.1 for error handling purposes */
    nVersion = OWS_1_1_1;

    if (getcontext_enabled==NULL || atoi(getcontext_enabled) == 0) {
      msSetError(MS_WMSERR, "GetContext not enabled on this server.",
                 "msWMSDispatch()");
      return msWMSException(map, nVersion, NULL, wms_exception_format);
    }

    if ((status = msOWSMakeAllLayersUnique(map)) != MS_SUCCESS)
      return msWMSException(map, nVersion, NULL, wms_exception_format);

    msIO_setHeader("Content-Type","text/xml; charset=UTF-8");
    msIO_sendHeaders();

    if ( msWriteMapContext(map, stdout) != MS_SUCCESS )
      return msWMSException(map, nVersion, NULL, wms_exception_format);
    /* Request completed */
    return MS_SUCCESS;
  } else if (request && strcasecmp(request, "GetMap") == 0 &&
             format && strcasecmp(format,  "image/txt") == 0) {
    /* Until someone adds full support for ASCII graphics this should do. ;) */
    msIO_setHeader("Content-Type","text/plain; charset=UTF-8");
    msIO_sendHeaders();
    msIO_printf(".\n               ,,ggddY\"\"\"Ybbgg,,\n          ,agd888b,_ "
                "\"Y8, ___'\"\"Ybga,\n       ,gdP\"\"88888888baa,.\"\"8b    \""
                "888g,\n     ,dP\"     ]888888888P'  \"Y     '888Yb,\n   ,dP\""
                "      ,88888888P\"  db,       \"8P\"\"Yb,\n  ,8\"       ,8888"
                "88888b, d8888a           \"8,\n ,8'        d88888888888,88P\""
                "' a,          '8,\n,8'         88888888888888PP\"  \"\"      "
                "     '8,\nd'          I88888888888P\"                   'b\n8"
                "           '8\"88P\"\"Y8P'                      8\n8         "
                "   Y 8[  _ \"                        8\n8              \"Y8d8"
                "b  \"Y a                   8\n8                 '\"\"8d,   __"
                "                 8\nY,                    '\"8bd888b,        "
                "     ,P\n'8,                     ,d8888888baaa       ,8'\n '8"
                ",                    888888888888'      ,8'\n  '8a           "
                "        \"8888888888I      a8'\n   'Yba                  'Y88"
                "88888P'    adP'\n     \"Yba                 '888888P'   adY\""
                "\n       '\"Yba,             d8888P\" ,adP\"' \n          '\""
                "Y8baa,      ,d888P,ad8P\"' \n               ''\"\"YYba8888P\""
                "\"''\n");
    return MS_SUCCESS;
  }

  /* If SERVICE, VERSION and REQUEST not included than this isn't a WMS req*/
  if (service == NULL && nVersion == OWS_VERSION_NOTSET && request==NULL)
    return MS_DONE;  /* Not a WMS request */

  /* VERSION *and* REQUEST required by both getMap and getFeatureInfo */
  if (nVersion == OWS_VERSION_NOTSET) {
    msSetError(MS_WMSERR,
               "Incomplete WMS request: VERSION parameter missing",
               "msWMSDispatch()");
    return msWMSException(map, OWS_VERSION_NOTSET, NULL, wms_exception_format);
  }

  /*check if the version is one of the supported vcersions*/
  if (nVersion != OWS_1_0_0 &&  nVersion != OWS_1_0_6 &&
      nVersion != OWS_1_0_7 && nVersion != OWS_1_1_0 &&
      nVersion != OWS_1_1_1 && nVersion != OWS_1_3_0) {
    msSetError(MS_WMSERR,
               "Invalid WMS version: VERSION %s is not supported. Supported versions are 1.0.0, 1.0.6, 1.0.7, 1.1.0, 1.1.1, 1.3.0",
               "msWMSDispatch()", version);
    return msWMSException(map, OWS_VERSION_NOTSET, NULL, wms_exception_format);
  }

  if (request==NULL) {
    msSetError(MS_WMSERR,
               "Incomplete WMS request: REQUEST parameter missing",
               "msWMSDispatch()");
    return msWMSException(map, nVersion, NULL, wms_exception_format);
  }

  /* hack !? The function can return MS_DONE ... be sure it's a wms request
   * before checking the enabled layers */
  if ( (strcasecmp(request, "GetStyles") == 0) ||
       (strcasecmp(request, "GetLegendGraphic") == 0) ||
       (strcasecmp(request, "GetSchemaExtension") == 0) ||
       (strcasecmp(request, "map") == 0 || strcasecmp(request, "GetMap") == 0) ||
       (strcasecmp(request, "feature_info") == 0 || strcasecmp(request, "GetFeatureInfo") == 0) ||
       (strcasecmp(request, "DescribeLayer") == 0) ) {
    char request_tmp[32];
    if (strcasecmp(request, "map") == 0)
      strlcpy(request_tmp, "GetMap", sizeof(request_tmp));
    else if (strcasecmp(request, "feature_info") == 0)
      strlcpy(request_tmp, "GetFeatureInfo", sizeof(request_tmp));
    else
      strlcpy(request_tmp, request, sizeof(request_tmp));

    msOWSRequestLayersEnabled(map, "M", request_tmp, ows_request);
    if (ows_request->numlayers == 0) {
      msSetError(MS_WMSERR, "WMS request not enabled. Check wms/ows_enable_request settings.", "msWMSDispatch()");
      return msWMSException(map, nVersion, NULL, wms_exception_format);
    }
  }

  if ((status = msOWSMakeAllLayersUnique(map)) != MS_SUCCESS)
    return msWMSException(map, nVersion, NULL, wms_exception_format);

  if (strcasecmp(request, "GetLegendGraphic") == 0) {
    /*
     * check for a BBOX in the request, in that case we have a content-dependant legend request,
     * and should be following the GetMap path a bit more
     */
    for(i=0; i<req->NumParams; i++) {
      if(strcasecmp(req->ParamNames[i], "BBOX") == 0) {
        if(req->ParamValues[i] && *req->ParamValues[i]) {
          break;
        }
      }
    }
    if(i != req->NumParams) {
      isContentDependantLegend = 1;
      /* getLegendGraphic uses LAYER= , we need to create a LAYERS= value that is identical
       * we'll suppose that the client is conformat and hasn't included a LAYERS= parameter
       * in its request */
      for(i=0; i<req->NumParams; i++) {
        if(strcasecmp(req->ParamNames[i], "LAYER") == 0) {
          req->ParamNames[req->NumParams] = msStrdup("LAYERS");
          req->ParamValues[req->NumParams] = msStrdup(req->ParamValues[i]);
          req->NumParams++;
        }
      }
    } else {
      return msWMSLegendGraphic(map, nVersion, req->ParamNames, req->ParamValues, req->NumParams,
                              wms_exception_format, ows_request, NULL);
    }
  }

  if (strcasecmp(request, "GetStyles") == 0)
    return msWMSGetStyles(map, nVersion, req->ParamNames, req->ParamValues, req->NumParams,
                          wms_exception_format);

  else if (request && strcasecmp(request, "GetSchemaExtension") == 0)
    return msWMSGetSchemaExtension(map);

  /* getMap parameters are used by both getMap, getFeatureInfo, and content dependant legendgraphics */
  if (strcasecmp(request, "map") == 0 || strcasecmp(request, "GetMap") == 0 ||
      strcasecmp(request, "feature_info") == 0 || strcasecmp(request, "GetFeatureInfo") == 0 || strcasecmp(request, "DescribeLayer") == 0 || isContentDependantLegend) {

    status = msWMSLoadGetMapParams(map, nVersion, req->ParamNames, req->ParamValues, req->NumParams,
                                   wms_exception_format, request, ows_request);
    if (status != MS_SUCCESS) return status;
  }

  /* This function owns validated_language, so remember to free it later*/
  validated_language = msOWSGetLanguageFromList(map, "MO", language);
  if (validated_language != NULL) {
    msMapSetLanguageSpecificConnection(map, validated_language);
  }
  msFree(validated_language);

  if (strcasecmp(request, "map") == 0 || strcasecmp(request, "GetMap") == 0)
    return msWMSGetMap(map, nVersion, req->ParamNames, req->ParamValues, req->NumParams, wms_exception_format, ows_request);
  else if (strcasecmp(request, "feature_info") == 0 || strcasecmp(request, "GetFeatureInfo") == 0)
    return msWMSFeatureInfo(map, nVersion, req->ParamNames, req->ParamValues, req->NumParams, wms_exception_format, ows_request);
  else if (strcasecmp(request, "DescribeLayer") == 0) {
    return msWMSDescribeLayer(map, nVersion, req->ParamNames, req->ParamValues, req->NumParams,
                              wms_exception_format);
  } else if (isContentDependantLegend) {
    return msWMSGetContentDependantLegend(map,nVersion, req->ParamNames, req->ParamValues, req->NumParams, wms_exception_format, ows_request);
  }

  /* Hummmm... incomplete or unsupported WMS request */
  if (service != NULL && strcasecmp(service, "WMS") == 0) {
    msSetError(MS_WMSERR, "Incomplete or unsupported WMS request", "msWMSDispatch()");
    return msWMSException(map, nVersion, NULL, wms_exception_format);
  } else
    return MS_DONE;  /* Not a WMS request */
#else
  msSetError(MS_WMSERR, "WMS server support is not available.", "msWMSDispatch()");
  return(MS_FAILURE);
#endif
}


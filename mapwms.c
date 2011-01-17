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

#include "mapserver.h"
#include "maperror.h"
#include "mapgml.h"
#include "gdfonts.h"

#include "maptemplate.h"

#include "mapogcsld.h"

#include "maptime.h"
#include "mapproject.h"

#include <stdarg.h>
#include <time.h>
#include <string.h>

#ifdef WIN32
#include <process.h>
#endif

MS_CVSID("$Id$")

/* ==================================================================
 * WMS Server stuff.
 * ================================================================== */
#ifdef USE_WMS_SVR

#ifdef USE_OGR
int ogrEnabled = 1;
#else
int ogrEnabled = 0;
#endif /* USE_OGR */

/*
** msWMSException()
**
** Report current MapServer error in requested format.
*/

int msWMSException(mapObj *map, int nVersion, const char *exception_code,  char *wms_exception_format)
{
    const char *encoding;
    char *schemalocation = NULL;

    /* Default to WMS 1.1.1 exceptions if version not set yet */
    if (nVersion <= 0)
        nVersion = OWS_1_1_1;

    /* get scheam location */
    schemalocation = msEncodeHTMLEntities( msOWSGetSchemasLocation(map) );

    encoding = msOWSLookupMetadata(&(map->web.metadata), "MO", "encoding");

  /* Establish default exception format depending on VERSION */
  if (wms_exception_format == NULL)
  {
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
      strcasecmp(wms_exception_format, "application/vnd.ogc.se_blank") == 0)
  {
    int blank = 0;

    if (strcasecmp(wms_exception_format, "BLANK") == 0 ||
        strcasecmp(wms_exception_format, "application/vnd.ogc.se_blank") == 0)
    {
        blank = 1;
    }

    msWriteErrorImage(map, NULL, blank);

  }
  else if (strcasecmp(wms_exception_format, "WMS_XML") == 0) /* Only in V1.0.0 */
  {
      if (encoding)
          msIO_printf("Content-type: text/xml; charset=%s%c%c", encoding,10,10);
      else
          msIO_printf("Content-type: text/xml%c%c",10,10);

    msIO_printf("<WMTException version=\"1.0.0\">\n");
    msWriteErrorXML(stdout);
    msIO_printf("</WMTException>\n");
  }
  else /* XML error, the default: SE_XML (1.0.1 to 1.0.7) */
       /* or application/vnd.ogc.se_xml (1.1.0 and later) */
  {
    if (nVersion <= OWS_1_0_7)
    {
      /* In V1.0.1 to 1.0.7, the MIME type was text/xml */
        if (encoding)
            msIO_printf("Content-type: text/xml; charset=%s%c%c", encoding,10,10);
        else
            msIO_printf("Content-type: text/xml%c%c",10,10);

      msOWSPrintEncodeMetadata(stdout, &(map->web.metadata),
                               "MO", "encoding", OWS_NOERR,
                "<?xml version='1.0' encoding=\"%s\" standalone=\"no\" ?>\n",
                    "ISO-8859-1");
      msIO_printf("<!DOCTYPE ServiceExceptionReport SYSTEM \"http://www.digitalearth.gov/wmt/xml/exception_1_0_1.dtd\">\n");

      msIO_printf("<ServiceExceptionReport version=\"1.0.1\">\n");
    }
    else if (nVersion <= OWS_1_1_0)
    {
      /* In V1.1.0 and later, we have OGC-specific MIME types */
      /* we cannot return anything else than application/vnd.ogc.se_xml here. */
        if (encoding)
            msIO_printf("Content-type: application/vnd.ogc.se_xml; charset=%s%c%c", encoding,10,10);
        else
            msIO_printf("Content-type: application/vnd.ogc.se_xml%c%c",10,10);
        
      msOWSPrintEncodeMetadata(stdout, &(map->web.metadata),
                               "MO", "encoding", OWS_NOERR,
                "<?xml version='1.0' encoding=\"%s\" standalone=\"no\" ?>\n",
                         "ISO-8859-1");

      
      msIO_printf("<!DOCTYPE ServiceExceptionReport SYSTEM \"%s/wms/1.1.0/exception_1_1_0.dtd\">\n",schemalocation);

      msIO_printf("<ServiceExceptionReport version=\"1.1.0\">\n");
    }
    else if (nVersion <= OWS_1_1_1)/* 1.1.1 */
    {
        if (encoding)
            msIO_printf("Content-type: application/vnd.ogc.se_xml; charset=%s%c%c", encoding,10,10);
        else
            msIO_printf("Content-type: application/vnd.ogc.se_xml%c%c",10,10);


        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata),
                                 "MO", "encoding", OWS_NOERR,
                  "<?xml version='1.0' encoding=\"%s\" standalone=\"no\" ?>\n",
                           "ISO-8859-1");
        msIO_printf("<!DOCTYPE ServiceExceptionReport SYSTEM \"%s/wms/1.1.1/exception_1_1_1.dtd\">\n", schemalocation);
        msIO_printf("<ServiceExceptionReport version=\"1.1.1\">\n");
    }
    else /*1.3.0*/
    {
        if (strcasecmp(wms_exception_format, "application/vnd.ogc.se_xml") == 0)
            if (encoding)
                msIO_printf("Content-type: application/vnd.ogc.se_xml; charset=%s%c%c", encoding,10,10);
            else
                msIO_printf("Content-type: application/vnd.ogc.se_xml%c%c",10,10);
        else
            if (encoding)
                msIO_printf("Content-type: text/xml; charset=%s%c%c", encoding,10,10);
            else
                msIO_printf("Content-type: text/xml%c%c",10,10);

        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata),
                                 "MO", "encoding", OWS_NOERR,
                  "<?xml version='1.0' encoding=\"%s\" standalone=\"no\" ?>\n",
                           "ISO-8859-1");
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

    free(schemalocation);

  }

  /* clear error since we have already reported it */
  msResetErrorList();

  return MS_FAILURE; /* so that we can call 'return msWMSException();' anywhere */
}



void msWMSSetTimePattern(const char *timepatternstring, char *timestring)
{
    char *time = NULL;
    char **atimes, **tokens = NULL;
    int numtimes, ntmp, i = 0;
    char *tmpstr = NULL;

    if (timepatternstring && timestring)
    {
        /* parse the time parameter to extract a distinct time. */
        /* time value can be dicrete times (eg 2004-09-21), */
        /* multiple times (2004-09-21, 2004-09-22, ...) */
        /* and range(s) (2004-09-21/2004-09-25, 2004-09-27/2004-09-29) */
        if (strstr(timestring, ",") == NULL &&
            strstr(timestring, "/") == NULL) /* discrete time */
        {
            time = strdup(timestring);
        }
        else
        {
            atimes = msStringSplit (timestring, ',', &numtimes);
            if (numtimes >=1 && atimes)
            {
                tokens = msStringSplit(atimes[0],  '/', &ntmp);
                if (ntmp == 2 && tokens) /* range */
                {
                    time = strdup(tokens[0]);
                }
                else /* multiple times */
                {
                    time = strdup(atimes[0]);
                }
                msFreeCharArray(tokens, ntmp);
                msFreeCharArray(atimes, numtimes);
            }
        }
        /* get the pattern to use */
        if (time)
        {
            tokens = msStringSplit(timepatternstring, ',', &ntmp);
            if (tokens && ntmp >= 1)
            {
                for (i=0; i<ntmp; i++)
                {
                  if (tokens[i] && strlen(tokens[i]) > 0)
                  {
                      msStringTrimBlanks(tokens[i]);
                      tmpstr = msStringTrimLeft(tokens[i]);
                      if (msTimeMatchPattern(time, tmpstr) == MS_TRUE)
                      {
                          msSetLimitedPattersToUse(tmpstr);
                          break;
                      }
                  }
                }
                msFreeCharArray(tokens, ntmp);
            }
            free(time);
        }

    }
}

/*
** Apply the TIME parameter to layers that are time aware
*/
int msWMSApplyTime(mapObj *map, int version, char *time, char *wms_exception_format)
{
    int i=0;
    layerObj *lp = NULL;
    const char *timeextent, *timefield, *timedefault, *timpattern = NULL;

    if (map)
    {
        for (i=0; i<map->numlayers; i++)
        {
            lp = (GET_LAYER(map, i));
            if (lp->status != MS_ON && lp->status != MS_DEFAULT)
              continue;
            /* check if the layer is time aware */
            timeextent = msOWSLookupMetadata(&(lp->metadata), "MO",
                                             "timeextent");
            timefield =  msOWSLookupMetadata(&(lp->metadata), "MO",
                                             "timeitem");
            timedefault =  msOWSLookupMetadata(&(lp->metadata), "MO",
                                             "timedefault");
            if (timeextent && timefield)
            {
                /* check to see if the time value is given. If not */
                /* use default time. If default time is not available */
                /* send an exception */
                if (time == NULL || strlen(time) <=0)
                {
                    if (timedefault == NULL)
                    {
                        msSetError(MS_WMSERR, "No Time value was given, and no default time value defined.", "msWMSApplyTime");
                        return msWMSException(map, version,
                                              "MissingDimensionValue", wms_exception_format);
                    }
                    else
                    {
                        if (msValidateTimeValue((char *)timedefault, timeextent) == MS_FALSE)
                        {
                            msSetError(MS_WMSERR, "No Time value was given, and the default time value %s is invalid or outside the time extent defined %s", "msWMSApplyTime", timedefault, timeextent);
                        /* return MS_FALSE; */
                            return msWMSException(map, version,
                                                  "InvalidDimensionValue", wms_exception_format);
                        }
                        msLayerSetTimeFilter(lp, timedefault, timefield);
                    }
                }
                else
                {
                    /* check if given time is in the range */
                    if (msValidateTimeValue(time, timeextent) == MS_FALSE)
                    {
                        if (timedefault == NULL)
                        {
                            msSetError(MS_WMSERR, "Time value(s) %s given is invalid or outside the time extent defined (%s).", "msWMSApplyTime", time, timeextent);
                        /* return MS_FALSE; */
                            return msWMSException(map, version,
                                                  "InvalidDimensionValue", wms_exception_format);
                        }
                        else
                        {
                            if (msValidateTimeValue((char *)timedefault, timeextent) == MS_FALSE)
                            {
                                msSetError(MS_WMSERR, "Time value(s) %s given is invalid or outside the time extent defined (%s), and default time set is invalid (%s)", "msWMSApplyTime", time, timeextent, timedefault);
                        /* return MS_FALSE; */
                                return msWMSException(map, version,
                                                      "InvalidDimensionValue", wms_exception_format);
                            }
                            else
                              msLayerSetTimeFilter(lp, timedefault, timefield);
                        }
                            
                    }
                    else
                    {
                    /* build the time string */
                        msLayerSetTimeFilter(lp, time, timefield);
                        timeextent= NULL;
                    }
                }
            }
        }
        /* check to see if there is a list of possible patterns defined */
        /* if it is the case, use it to set the time pattern to use */
        /* for the request */

        timpattern = msOWSLookupMetadata(&(map->web.metadata), "MO",
                                         "timeformat");
        if (timpattern && time && strlen(time) > 0)
          msWMSSetTimePattern(timpattern, time);
    }

    return MS_SUCCESS;
}


/*
**
*/
int msWMSLoadGetMapParams(mapObj *map, int nVersion,
                          char **names, char **values, int numentries, char *wms_exception_format)
{
  int i, adjust_extent = MS_FALSE, nonsquare_enabled = MS_FALSE;
  int iUnits = -1;
  int nLayerOrder = 0;
  int transparent = MS_NOOVERRIDE;
  outputFormatObj *format = NULL;
  int validlayers = 0;
  char *styles = NULL;
  int numlayers = 0;
  char **layers = NULL;
  int layerfound =0;
  int invalidlayers = 0;
  char epsgbuf[100];
  char srsbuffer[100];
  int epsgvalid = MS_FALSE;
  const char *projstring;
   char **tokens;
   int n,j = 0;
   int timerequest = 0;
   char *stime = NULL;

  int srsfound = 0;
  int bboxfound = 0;
  int formatfound = 0;
  int widthfound = 0;
  int heightfound = 0;

  char *request = NULL;
  int status = 0;

  char *bboxrequest=NULL;
  const char *sldenabled=NULL;

   epsgbuf[0]='\0';
   srsbuffer[0]='\0';

  /* Some of the getMap parameters are actually required depending on the */
  /* request, but for now we assume all are optional and the map file */
  /* defaults will apply. */

   msAdjustExtent(&(map->extent), map->width, map->height);

   for(i=0; map && i<numentries; i++)
   {
    /* getMap parameters */

    if (strcasecmp(names[i], "REQUEST") == 0)
    {
      request = values[i];
    }

    /* check if SLD is passed.  If yes, check for OGR support */
    if (strcasecmp(names[i], "SLD") == 0 || strcasecmp(names[i], "SLD_BODY") == 0)
    {
       sldenabled = msOWSLookupMetadata(&(map->web.metadata), "MO", "sld_enabled");

       if (sldenabled == NULL)
         sldenabled = strdup("true");

       if (ogrEnabled == 0)
       {
          msSetError(MS_WMSERR, "OGR support is not available.", "msWMSLoadGetMapParams()");
          return msWMSException(map, nVersion, NULL, wms_exception_format);
       }
       else
       {
          if (strcasecmp(sldenabled, "true") == 0)
          {
            if (strcasecmp(names[i], "SLD") == 0)
            {
              if ((status = msSLDApplySLDURL(map, values[i], -1, NULL)) != MS_SUCCESS)
                return msWMSException(map, nVersion, NULL, wms_exception_format);
            }
            if (strcasecmp(names[i], "SLD_BODY") == 0)
            {
              if ((status =msSLDApplySLD(map, values[i], -1, NULL)) != MS_SUCCESS)
                return msWMSException(map, nVersion, NULL, wms_exception_format);
            }
          }
       }
    }

    if (strcasecmp(names[i], "LAYERS") == 0)
    {
      int  j, k, iLayer, *layerOrder;
      
      layerOrder = (int*)malloc(map->numlayers * sizeof(int));
      if (layerOrder == NULL)
      {
        msSetError(MS_MEMERR, NULL, "msWMSLoadGetMapParams()");
        return MS_FAILURE;
      }

      layers = msStringSplit(values[i], ',', &numlayers);
      if (layers==NULL || strlen(values[i]) <=0 ||   numlayers < 1) {
        msSetError(MS_WMSERR, "At least one layer name required in LAYERS.",
                   "msWMSLoadGetMapParams()");
        return msWMSException(map, nVersion, NULL, wms_exception_format);
      }


      for (iLayer=0; iLayer < map->numlayers; iLayer++)
      {
          map->layerorder[iLayer] = iLayer;
          layerOrder[iLayer] = 0;
      }

      for(j=0; j<map->numlayers; j++)
      {
        /* Keep only layers with status=DEFAULT by default */
        /* Layer with status DEFAULT is drawn first. */
        if (GET_LAYER(map, j)->status != MS_DEFAULT)
           GET_LAYER(map, j)->status = MS_OFF;
        else
        {
           map->layerorder[nLayerOrder++] = j;
           layerOrder[j] = 1;
        }
      }

      for (k=0; k<numlayers; k++)
      {
          layerfound = 0;
          for (j=0; j<map->numlayers; j++)
          {
              /* Turn on selected layers only. */
              if ((GET_LAYER(map, j)->name &&
                   strcasecmp(GET_LAYER(map, j)->name, layers[k]) == 0) ||
                  (map->name && strcasecmp(map->name, layers[k]) == 0) ||
                  (GET_LAYER(map, j)->group && strcasecmp(GET_LAYER(map, j)->group, layers[k]) == 0))
              {
                  if (GET_LAYER(map, j)->status != MS_DEFAULT)
                  {
                     if (layerOrder[j] == 0)
                     {
                        map->layerorder[nLayerOrder++] = j;
                        layerOrder[j] = 1;
                        GET_LAYER(map, j)->status = MS_ON;
                     }
                  }
                  validlayers++;
                  layerfound = 1;
              }
          }
          if (layerfound == 0)
            invalidlayers++;

      }

      /* set all layers with status off at end of array */
      for (j=0; j<map->numlayers; j++)
      {
         if (GET_LAYER(map, j)->status == MS_OFF)
            if (layerOrder[j] == 0)
               map->layerorder[nLayerOrder++] = j;
      }
      
      free(layerOrder);
      msFreeCharArray(layers, numlayers);
    }
    else if (strcasecmp(names[i], "STYLES") == 0) {
        styles = values[i];

    }
    else if ((strcasecmp(names[i], "SRS") == 0 && nVersion < OWS_1_3_0) || 
             (strcasecmp(names[i], "CRS") == 0 && nVersion >= OWS_1_3_0)) {
      srsfound = 1;
      /* SRS is in format "EPSG:epsg_id" or "AUTO:proj_id,unit_id,lon0,lat0" */
      if (strncasecmp(values[i], "EPSG:", 5) == 0)
      {
        /* SRS=EPSG:xxxx */
        
        /*don't need to copy init=xxx since the srsbudder is only
          used with msLoadProjection and that does alreay the job*/
        //snprintf(srsbuffer, 100, "init=epsg:%.20s", values[i]+5);

        
        snprintf(srsbuffer, 100, "EPSG:%.20s",values[i]+5);
        snprintf(epsgbuf, 100, "EPSG:%.20s",values[i]+5);
       
        /* This test was to correct a request by the OCG cite 1.3.0 test
         sending CRS=ESPG:4326,  Bug:*/
        if (nVersion >= OWS_1_3_0)
        {
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
      }
      else if (strncasecmp(values[i], "AUTO:", 5) == 0 && nVersion < OWS_1_3_0)
      {
        snprintf(srsbuffer, 100, "%s",  values[i]);
        /* SRS=AUTO:proj_id,unit_id,lon0,lat0 */
        /*
        if (msLoadProjectionString(&(map->projection), values[i]) != 0)
          return msWMSException(map, nVersion, NULL);

        iUnits = GetMapserverUnitUsingProj(&(map->projection));
        if (iUnits != -1)
          map->units = iUnits;
        */
      }
      else if (nVersion >= OWS_1_3_0 && (strncasecmp(values[i], "AUTO2:", 6) == 0 ||
                                         strncasecmp(values[i], "CRS:", 4) == 0))
      {
          snprintf(srsbuffer, 100, "%s",  values[i]);
      }
      else
      {
          if (nVersion >= OWS_1_3_0)
          {
              msSetError(MS_WMSERR,
                         "Unsupported CRS namespace (only EPSG, AUTO2, CRS currently supported).",
                         "msWMSLoadGetMapParams()");
              return msWMSException(map, nVersion, "InvalidCRS", wms_exception_format);
          }
          else
          {
              msSetError(MS_WMSERR,
                         "Unsupported SRS namespace (only EPSG and AUTO currently supported).",
                         "msWMSLoadGetMapParams()");
              return msWMSException(map, nVersion, "InvalidSRS", wms_exception_format);
          }
      }
    }
    else if (strcasecmp(names[i], "BBOX") == 0) {
      char **tokens = 0;
      int n = 0;
      bboxfound = 1;
      tokens = msStringSplit(values[i], ',', &n);
      if (tokens==NULL || n != 4) {
        msSetError(MS_WMSERR, "Wrong number of arguments for BBOX.",
                   "msWMSLoadGetMapParams()");
        return msWMSException(map, nVersion, NULL, wms_exception_format);
      }
      bboxrequest = values[i];

      /*for wms 1.3.0 we will do the validation of the bbox after all parameters
       are read to account for the axes order*/
      if (nVersion < OWS_1_3_0)
      {
          map->extent.minx = atof(tokens[0]);
          map->extent.miny = atof(tokens[1]);
          map->extent.maxx = atof(tokens[2]);
          map->extent.maxy = atof(tokens[3]);

          msFreeCharArray(tokens, n);

          /* validate bbox values */
          if ( map->extent.minx >= map->extent.maxx ||
               map->extent.miny >=  map->extent.maxy)
          {
              msSetError(MS_WMSERR,
                         "Invalid values for BBOX.",
                         "msWMSLoadGetMapParams()");
              return msWMSException(map, nVersion, NULL, wms_exception_format);
          }
          adjust_extent = MS_TRUE;
      }
    }
    else if (strcasecmp(names[i], "WIDTH") == 0) {
      widthfound = 1;
      map->width = atoi(values[i]);
    }
    else if (strcasecmp(names[i], "HEIGHT") == 0) {
      heightfound = 1;
      map->height = atoi(values[i]);
    }
    else if (strcasecmp(names[i], "FORMAT") == 0) {
      const char *format_list = NULL;
      char **tokens = NULL;
	
      formatfound = 1;

      /*check to see if a predefined list is given*/
      format_list = msOWSLookupMetadata(&(map->web.metadata), "M","getmap_formatlist");
      n = 0;
      if ( format_list)
        tokens = msStringSplit(format_list,  ',', &n);
      
      if (tokens && n > 0)
      {
          for (j=0; j<n; j++)
          {
              if (strcasecmp(tokens[j], values[i]) == 0)
                break;
          }
          msFreeCharArray(tokens, n);
          if (j == n || (format = msSelectOutputFormat( map, values[i])) == NULL)
          {
            msSetError(MS_IMGERR,
                       "Unsupported output format (%s).",
                       "msWMSLoadGetMapParams()",
                       values[i] );
            return msWMSException(map, nVersion, "InvalidFormat", wms_exception_format);
          }
      }
      else
      {
          format = msSelectOutputFormat( map, values[i] );
          if( format == NULL || 
              (strncasecmp(format->driver, "GD/", 3) != 0 &&
               strncasecmp(format->driver, "GDAL/", 5) != 0 && 
               strncasecmp(format->driver, "AGG/", 4) != 0 &&
               strncasecmp(format->driver, "SVG", 3) != 0))
          {
              msSetError(MS_IMGERR,
                         "Unsupported output format (%s).",
                         "msWMSLoadGetMapParams()",
                         values[i] );
              return msWMSException(map, nVersion, "InvalidFormat", wms_exception_format);
          }
      }
      msFree( map->imagetype );
      map->imagetype = strdup(values[i]);
    }
    else if (strcasecmp(names[i], "TRANSPARENT") == 0) {
      transparent = (strcasecmp(values[i], "TRUE") == 0);
    }
    else if (strcasecmp(names[i], "BGCOLOR") == 0) {
      long c;
      c = strtol(values[i], NULL, 16);
      map->imagecolor.red = (c/0x10000)&0xff;
      map->imagecolor.green = (c/0x100)&0xff;
      map->imagecolor.blue = c&0xff;
    }

    /* value of time can be empty. We should look for a default value */
    /* see function msWMSApplyTime */
    else if (strcasecmp(names[i], "TIME") == 0)/* &&  values[i]) */
    {
        stime = values[i];
        timerequest = 1;
    }
  }

   /*validate the exception format WMS 1.3.0 section 7.3.3.11*/

   if (nVersion >= OWS_1_3_0 && wms_exception_format != NULL)
   {    
       if (strcasecmp(wms_exception_format, "INIMAGE") != 0 &&
           strcasecmp(wms_exception_format, "BLANK") != 0 &&
           strcasecmp(wms_exception_format, "XML") != 0)
       {
           msSetError(MS_WMSERR,
                      "Invalid format for the EXCEPTIONS parameter.",
                      "msWMSLoadGetMapParams()");
           return msWMSException(map, nVersion, "InvalidFormat", wms_exception_format);
       }
   }
   if (bboxfound && bboxrequest && nVersion >= OWS_1_3_0)
   {
       char **tokens;
       int n;
       rectObj rect;
       projectionObj proj;
       tokens = msStringSplit(bboxrequest, ',', &n);
       
       /*we have already validated that the request format when reding
        the request parameters*/
       rect.minx = atof(tokens[0]);
       rect.miny = atof(tokens[1]);
       rect.maxx = atof(tokens[2]);
       rect.maxy = atof(tokens[3]);
   
       msFreeCharArray(tokens, n);

       /*try to adjust the axes if necessary*/
       if (strlen(srsbuffer) > 1)
       {
            msInitProjection(&proj);
            if (msLoadProjectionStringEPSG(&proj, (char *)srsbuffer) == 0)
            {
                msAxisNormalizePoints( &proj, 1, &rect.minx, &rect.miny );
                msAxisNormalizePoints( &proj, 1, &rect.maxx, &rect.maxy );
            }
            msFreeProjection( &proj );
       }
       /*if the CRS is AUTO2:auto_crs_id,factor,lon0,lat0,
        we need to grab the factor patameter and use it with the bbox*/
       if (strlen(srsbuffer) > 1 && strncasecmp(srsbuffer, "AUTO2:", 6) == 0)
       {
           char **args;
           int numargs;
           double factor;
           args = msStringSplit(srsbuffer, ',', &numargs);
           if (numargs == 4)
           {
               factor = atof(args[1]);
               if (factor > 0 && factor != 1.0)
               {
                   rect.minx = rect.minx*factor;
                   rect.miny = rect.miny*factor;
                   rect.maxx = rect.maxx*factor;
                   rect.maxx = rect.maxy*factor;
               }
           }
           msFreeCharArray(args, numargs);
       }

       map->extent.minx = rect.minx;
       map->extent.miny = rect.miny;
       map->extent.maxx = rect.maxx;
       map->extent.maxy = rect.maxy;

       /* validate bbox values */
       if ( map->extent.minx >= map->extent.maxx ||
            map->extent.miny >=  map->extent.maxy)
       {
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
  if( !timerequest && map )
  {
      for (i=0; i<map->numlayers && !timerequest; i++)
      {
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
  if (timerequest)
  {
    if (msWMSApplyTime(map, nVersion, stime, wms_exception_format) == MS_FAILURE)
      {
          return MS_FAILURE;/* msWMSException(map, nVersion, "InvalidTimeRequest"); */
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
  if (validlayers == 0 || invalidlayers > 0)
  {
      msSetError(MS_WMSERR, "Invalid layer(s) given in the LAYERS parameter.",
                 "msWMSLoadGetMapParams()");
      return msWMSException(map, nVersion, "LayerNotDefined", wms_exception_format);
  }

  /* validate srs value: When the SRS parameter in a GetMap request contains a
  ** SRS that is valid for some, but not all of the layers being requested, 
  ** then the server shall throw a Service Exception (code = "InvalidSRS").
  ** Validate first against epsg in the map and if no matching srs is found
  ** validate all layers requested.
  */
  if (epsgbuf != NULL && strlen(epsgbuf) > 1)
  {
      epsgvalid = MS_FALSE;
      projstring = msOWSGetEPSGProj(&(map->projection), &(map->web.metadata),
                                    "MO", MS_FALSE);
      if (projstring)
      {
          tokens = msStringSplit(projstring, ' ', &n);
          if (tokens && n > 0)
          {
              for(i=0; i<n; i++)
              {
                  if (strcasecmp(tokens[i], epsgbuf) == 0)
                  {
                      epsgvalid = MS_TRUE;
                      break;
                  }
              }
              msFreeCharArray(tokens, n);
          }
      }
      if (epsgvalid == MS_FALSE)
      {
          for (i=0; i<map->numlayers; i++)
          {
              epsgvalid = MS_FALSE;
              if (GET_LAYER(map, i)->status == MS_ON)
              {
                  projstring = msOWSGetEPSGProj(&(GET_LAYER(map, i)->projection),
                                                &(GET_LAYER(map, i)->metadata),
                                                "MO", MS_FALSE);
                  if (projstring)
                  {
                      tokens = msStringSplit(projstring, ' ', &n);
                      if (tokens && n > 0)
                      {
                          for(j=0; j<n; j++)
                          {
                              if (strcasecmp(tokens[j], epsgbuf) == 0)
                              {
                                  epsgvalid = MS_TRUE;
                                  break;
                              }
                          }
                          msFreeCharArray(tokens, n);
                      }
                  }
                  if (epsgvalid == MS_FALSE)
                  {
                      if (nVersion >= OWS_1_3_0)
                      {
                          msSetError(MS_WMSERR, "Invalid CRS given : CRS must be valid for all requested layers.",
                                     "msWMSLoadGetMapParams()");
                          return msWMSException(map, nVersion, "InvalidSRS", wms_exception_format);
                      }
                      else
                      {
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
     map->width < 1 || map->height < 1)
  {
      msSetError(MS_WMSERR, "Image size out of range, WIDTH and HEIGHT must be between 1 and %d pixels.", "msWMSLoadGetMapParams()", map->maxsize);

      /* Restore valid default values in case errors INIMAGE are used */
      map->width = 400;
      map->height= 300;
      return msWMSException(map, nVersion, NULL, wms_exception_format);
  }

  /* Check whether requested BBOX and width/height result in non-square pixels
   */
  nonsquare_enabled = msTestConfigOption( map, "MS_NONSQUARE", MS_FALSE );
  if (!nonsquare_enabled)
  {
      double dx, dy, reqy;
      dx = MS_ABS(map->extent.maxx - map->extent.minx);
      dy = MS_ABS(map->extent.maxy - map->extent.miny);

      reqy = ((double)map->width) * dy / dx;

      /* Allow up to 1 pixel of error on the width/height ratios. */
      /* If more than 1 pixel then enable non-square pixels */
      if ( MS_ABS((reqy - (double)map->height)) > 1.0 )
      {
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
  if ((srsbuffer != NULL && strlen(srsbuffer) > 1) || nonsquare_enabled)
  {
      projectionObj newProj;

      if (map->projection.numargs <= 0)
      {
          if (nVersion >= OWS_1_3_0)
          {
              msSetError(MS_WMSERR, "Cannot set new CRS on a map that doesn't "
                         "have any projection set. Please make sure your mapfile "
                         "has a projection defined at the top level.", 
                         "msWMSLoadGetMapParams()");
              return msWMSException(map, nVersion, "InvalidCRS", wms_exception_format);
          }
          else
          {
               msSetError(MS_WMSERR, "Cannot set new SRS on a map that doesn't "
                         "have any projection set. Please make sure your mapfile "
                         "has a projection defined at the top level.", 
                         "msWMSLoadGetMapParams()");
               return msWMSException(map, nVersion, "InvalidSRS", wms_exception_format);
          }
      }
      
      msInitProjection(&newProj);
      if (srsbuffer != NULL && strlen(srsbuffer) > 1)
      {
          int nTmp;
          
          if (nVersion >= OWS_1_3_0)   
            nTmp = msLoadProjectionStringEPSG(&newProj, srsbuffer);
          else 
            nTmp = msLoadProjectionString(&newProj, srsbuffer);
          if (nTmp != 0)
          { 
              msFreeProjection(&newProj);
              return msWMSException(map, nVersion, NULL, wms_exception_format);
          }
      }

      if (nonsquare_enabled || 
          msProjectionsDiffer(&(map->projection), &newProj))
      {
          char *original_srs = NULL;

          for(i=0; i<map->numlayers; i++)
          {
              if (GET_LAYER(map, i)->projection.numargs <= 0 &&
                  GET_LAYER(map, i)->status != MS_OFF &&
                  GET_LAYER(map, i)->transform == MS_TRUE)
              {
                  /* This layer is turned on and needs a projection */

                  /* Fetch main map projection string only now that we need it */
                  if (original_srs == NULL)
                      original_srs = msGetProjectionString(&(map->projection));

                  if (msLoadProjectionString(&(GET_LAYER(map, i)->projection),
                                             original_srs) != 0)
                  {
                      msFreeProjection(&newProj);
                      return msWMSException(map, nVersion, NULL, wms_exception_format);
                  }
                  GET_LAYER(map, i)->project = MS_TRUE;
              }
          }

          msFree(original_srs);
      }

      msFreeProjection(&newProj);
  }

  /* apply the srs to the map file. This is only done after validating */
  /* that the srs given as parameter is valid for all layers */
  if (srsbuffer != NULL && strlen(srsbuffer) > 1)
  {
      int nTmp;
      msFreeProjection(&map->projection);
      msInitProjection(&map->projection);
      if (nVersion >= OWS_1_3_0)
        nTmp = msLoadProjectionStringEPSG(&(map->projection), srsbuffer);
      else
        nTmp = msLoadProjectionString(&(map->projection), srsbuffer);

      if (nTmp != 0)
        return msWMSException(map, nVersion, NULL, wms_exception_format);

      iUnits = GetMapserverUnitUsingProj(&(map->projection));
      if (iUnits != -1)
        map->units = iUnits;
  }

  /* Validate Styles :
  ** mapserv does not advertize any styles (the default styles are the
  ** one that are used. So we are expecting here to have empty values
  ** for the styles parameter (...&STYLES=&...) Or for multiple Styles/Layers,
  ** we could have ...&STYLES=,,,. If that is not the
  ** case, we generate an exception.
  */
  if(styles && strlen(styles) > 0)
  {
      char **tokens;
      int n=0, i=0, k=0;
      char **layers=NULL;
      int numlayers =0;
      layerObj *lp = NULL;

      tokens = msStringSplit(styles, ',' ,&n);
      for (i=0; i<n; i++)
      {
          if (tokens[i] && strlen(tokens[i]) > 0 && 
              strcasecmp(tokens[i],"default") != 0)
          {
              if (layers == NULL)
              {
                  for(j=0; j<numentries; j++)
                  {     
                      if (strcasecmp(names[j], "LAYERS") == 0)
                      {
                          layers = msStringSplit(values[j], ',', &numlayers);
                      }
                  }
              }
              if (layers && numlayers == n)
              {
                  for (j=0; j<map->numlayers; j++)
                  {
                      if ((GET_LAYER(map, j)->name &&
                           strcasecmp(GET_LAYER(map, j)->name, layers[i]) == 0) ||
                          (map->name && strcasecmp(map->name, layers[i]) == 0) ||
                          (GET_LAYER(map, j)->group && strcasecmp(GET_LAYER(map, j)->group, layers[i]) == 0))
                      {
                          lp =   GET_LAYER(map, j);
                          for (k=0; k<lp->numclasses; k++)
                          {
                              if (lp->class[k]->group && strcasecmp(lp->class[k]->group, tokens[i]) == 0)
                              {
                                  if (lp->classgroup)
                                    msFree(lp->classgroup);
                                  lp->classgroup = strdup( tokens[i]);
                                  break;
                              }
                          }
                          if (k == lp->numclasses)
                          {
                              msSetError(MS_WMSERR, "Style (%s) not defined on layer.",
                                         "msWMSLoadGetMapParams()", tokens[i]);
                              msFreeCharArray(tokens, n);
                              msFreeCharArray(layers, numlayers);
                              
                              return msWMSException(map, nVersion, "StyleNotDefined", wms_exception_format);
                          }
                      }
                                     
                  }
              }
              else
              {
                  msSetError(MS_WMSERR, "Invalid style (%s). Mapserver is expecting an empty string for the STYLES : STYLES= or STYLES=,,, or using keyword default  STYLES=default,default, ...",
                             "msWMSLoadGetMapParams()", styles);
                  if (tokens && n > 0)
                    msFreeCharArray(tokens, n);
                  if (layers && numlayers > 0)
                    msFreeCharArray(layers, numlayers);
                  return msWMSException(map, nVersion, "StyleNotDefined", wms_exception_format);
              }
          }
      }
      if (tokens && n > 0)
        msFreeCharArray(tokens, n);
  }

  /*
  ** WMS extents are edge to edge while MapServer extents are center of
  ** pixel to center of pixel.  Here we try to adjust the WMS extents
  ** in by half a pixel.  We wait till here because we want to ensure we
  ** are doing this in terms of the correct WIDTH and HEIGHT.
  */
  if( adjust_extent )
  {
      double	dx, dy;

      dx = (map->extent.maxx - map->extent.minx) / map->width;
      map->extent.minx += dx*0.5;
      map->extent.maxx -= dx*0.5;

      dy = (map->extent.maxy - map->extent.miny) / map->height;
      map->extent.miny += dy*0.5;
      map->extent.maxy -= dy*0.5;
  }

  if (request && strcasecmp(request, "DescribeLayer") != 0) {
    if (srsfound == 0)
    {
      if ( nVersion >= OWS_1_3_0)
        msSetError(MS_WMSERR, "Missing required parameter CRS", "msWMSLoadGetMapParams()");
      else
        msSetError(MS_WMSERR, "Missing required parameter SRS", "msWMSLoadGetMapParams()");

      return msWMSException(map, nVersion, "MissingParameterValue", wms_exception_format);
    }

    if (bboxfound == 0)
    {
      msSetError(MS_WMSERR, "Missing required parameter BBOX", "msWMSLoadGetMapParams()");
      return msWMSException(map, nVersion, "MissingParameterValue", wms_exception_format);
    }  

    if (formatfound == 0 && (strcasecmp(request, "GetMap") == 0 || strcasecmp(request, "map") == 0))
    {
      msSetError(MS_WMSERR, "Missing required parameter FORMAT", "msWMSLoadGetMapParams()");
      return msWMSException(map, nVersion, "MissingParameterValue", wms_exception_format);
    }

    if (widthfound == 0)
    {
      msSetError(MS_WMSERR, "Missing required parameter WIDTH", "msWMSLoadGetMapParams()");
      return msWMSException(map, nVersion, "MissingParameterValue", wms_exception_format);
    }

    if (heightfound == 0)
    {
      msSetError(MS_WMSERR, "Missing required parameter HEIGHT", "msWMSLoadGetMapParams()");
      return msWMSException(map, nVersion, "MissingParameterValue", wms_exception_format);
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
  while(fmt != NULL)
  {
      /* Special case for early WMS with subelements in Format (bug 908) */
      if( nVersion <= OWS_1_0_7 )
      {
          encoded = strdup( fmt );
      }

      /* otherwise we HTML code special characters */
      else
      {
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

    if (stream && metadata)
    {
        title = msOWSLookupMetadata(metadata, "MO", 
                                  "attribution_title");
        onlineres = msOWSLookupMetadata(metadata, "MO", 
                                  "attribution_onlineresource");
        logourl = msOWSLookupMetadata(metadata, "MO", 
                                  "attribution_logourl_width");

        if (title || onlineres || logourl)
        {
            msIO_printf("%s<Attribution>\n",tabspace);
            if (title)
            {
                pszEncodedValue = msEncodeHTMLEntities(title);
                msIO_fprintf(stream, "%s%s<Title>%s</Title>\n", tabspace,
                             tabspace, pszEncodedValue);
                free(pszEncodedValue);
            }

            if (onlineres)
            {
                pszEncodedValue = msEncodeHTMLEntities(onlineres);
                msIO_fprintf(stream, "%s%s<OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:href=\"%s\"/>\n", tabspace, tabspace,
                             pszEncodedValue);
                free(pszEncodedValue);
            }
         
            if (logourl)
            {
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

  if (scalehintmin > 0.0 || scalehintmax > 0.0)
  {
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
** msDumpLayer()
*/
int msDumpLayer(mapObj *map, layerObj *lp, int nVersion, const char *script_url_encoded, const char *indent)
{
   rectObj ext;
   const char *value;
   const char *pszWmsTimeExtent, *pszWmsTimeDefault= NULL, *pszStyle=NULL;
   const char *pszLegendURL=NULL;
   char *pszMetadataName=NULL, *mimetype=NULL;
   char **classgroups = NULL;
   int iclassgroups=0 ,j=0;
   char szVersionBuf[OWS_VERSION_MAXLEN];

   /* if the layer status is set to MS_DEFAULT, output a warning */
   if (lp->status == MS_DEFAULT)
     msIO_fprintf(stdout, "<!-- WARNING: This layer has its status set to DEFAULT and will always be displayed when doing a GetMap request even if it is not requested by the client. This is not in line with the expected behavior of a WMS server. Using status ON or OFF is recommended. -->\n");
 
   if (nVersion <= OWS_1_0_7)
   {
       msIO_printf("%s    <Layer queryable=\"%d\">\n",
              indent, msIsLayerQueryable(lp));
   }
   else
   {
       /* 1.1.0 and later: opaque and cascaded are new. */
       int cascaded=0, opaque=0;
       if ((value = msOWSLookupMetadata(&(lp->metadata), "MO", "opaque")) != NULL)
           opaque = atoi(value);
       if (lp->connectiontype == MS_WMS)
           cascaded = 1;

       msIO_printf("%s    <Layer queryable=\"%d\" opaque=\"%d\" cascaded=\"%d\">\n",
              indent, msIsLayerQueryable(lp), opaque, cascaded);
   }
   
   if (lp->name && strlen(lp->name) > 0 &&
       (msIsXMLTagValid(lp->name) == MS_FALSE || isdigit(lp->name[0])))
     msIO_fprintf(stdout, "<!-- WARNING: The layer name '%s' might contain spaces or "
                        "invalid characters or may start with a number. This could lead to potential problems. -->\n", 
                  lp->name);
   msOWSPrintEncodeParam(stdout, "LAYER.NAME", lp->name, OWS_NOERR,
                         "        <Name>%s</Name>\n", NULL);

   /* the majority of this section is dependent on appropriately named metadata in the LAYER object */
   msOWSPrintEncodeMetadata(stdout, &(lp->metadata), "MO", "title",
                            OWS_WARN, "        <Title>%s</Title>\n", lp->name);

   msOWSPrintEncodeMetadata(stdout, &(lp->metadata), "MO", "abstract",
                         OWS_NOERR, "        <Abstract>%s</Abstract>\n", NULL);

   if (nVersion == OWS_1_0_0)
   {
       /* <Keywords> in V 1.0.0 */
       /* The 1.0.0 spec doesn't specify which delimiter to use so let's use spaces */
       msOWSPrintEncodeMetadataList(stdout, &(lp->metadata), "MO",
                                    "keywordlist",
                                    "        <Keywords>",
                                    "        </Keywords>\n",
                                    "%s ", NULL);
   }
   else
   {
       /* <KeywordList><Keyword> ... in V1.0.6+ */
       msOWSPrintEncodeMetadataList(stdout, &(lp->metadata), "MO",
                                    "keywordlist",
                                    "        <KeywordList>\n",
                                    "        </KeywordList>\n",
                                    "          <Keyword>%s</Keyword>\n", NULL);
   }

   if (msOWSGetEPSGProj(&(map->projection),&(map->web.metadata),
                        "MO", MS_FALSE) == NULL)
   {
       /* If map has no proj then every layer MUST have one or produce a warning */
       if (nVersion > OWS_1_1_0)
       {
           /* starting 1.1.1 SRS are given in individual tags */
           if (nVersion >= OWS_1_3_0)
             msOWSPrintEncodeParamList(stdout, "(at least one of) "
                                       "MAP.PROJECTION, LAYER.PROJECTION "
                                       "or wms_srs metadata", 
                                       msOWSGetEPSGProj(&(lp->projection), 
                                                      &(lp->metadata),
                                                      "MO", MS_FALSE),
                                     OWS_WARN, ' ', NULL, NULL, 
                                     "        <CRS>%s</CRS>\n", NULL);
           else
             msOWSPrintEncodeParamList(stdout, "(at least one of) "
                                       "MAP.PROJECTION, LAYER.PROJECTION "
                                       "or wms_srs metadata", 
                                       msOWSGetEPSGProj(&(lp->projection), 
                                                      &(lp->metadata),
                                                      "MO", MS_FALSE),
                                     OWS_WARN, ' ', NULL, NULL, 
                                     "        <SRS>%s</SRS>\n", NULL);
       }
       else
         msOWSPrintEncodeParam(stdout, "(at least one of) MAP.PROJECTION, "
                               "LAYER.PROJECTION or wms_srs metadata",
                               msOWSGetEPSGProj(&(lp->projection),
                                                &(lp->metadata), "MO", MS_FALSE),
                               OWS_WARN, "        <SRS>%s</SRS>\n", NULL);
   }
   else
   {
       /* No warning required in this case since there's at least a map proj. */
       if (nVersion > OWS_1_1_0)
       {
           /* starting 1.1.1 SRS are given in individual tags */
           if (nVersion >=  OWS_1_3_0)
             msOWSPrintEncodeParamList(stdout, "(at least one of) "
                                     "MAP.PROJECTION, LAYER.PROJECTION "
                                     "or wms_srs metadata", 
                                     msOWSGetEPSGProj(&(lp->projection), 
                                                      &(lp->metadata),
                                                      "MO", MS_FALSE),
                                     OWS_NOERR, ' ', NULL, NULL, 
                                     "        <CRS>%s</CRS>\n", NULL);
           else
             msOWSPrintEncodeParamList(stdout, "(at least one of) "
                                     "MAP.PROJECTION, LAYER.PROJECTION "
                                     "or wms_srs metadata", 
                                     msOWSGetEPSGProj(&(lp->projection), 
                                                      &(lp->metadata),
                                                      "MO", MS_FALSE),
                                     OWS_NOERR, ' ', NULL, NULL, 
                                     "        <SRS>%s</SRS>\n", NULL);
       }
       else
         msOWSPrintEncodeParam(stdout,
                               " LAYER.PROJECTION (or wms_srs metadata)",
                               msOWSGetEPSGProj(&(lp->projection),
                                                &(lp->metadata),"MO",MS_FALSE),
                               OWS_NOERR, "        <SRS>%s</SRS>\n", NULL);
   }

   /* If layer has no proj set then use map->proj for bounding box. */
   if (msOWSGetLayerExtent(map, lp, "MO", &ext) == MS_SUCCESS)
   {
       if(lp->projection.numargs > 0)
       {
           if (nVersion >= OWS_1_3_0)
             msOWSPrintEX_GeographicBoundingBox(stdout, "    ", &(ext),
                                                &(lp->projection));
           else
             msOWSPrintLatLonBoundingBox(stdout, "        ", &(ext),
                                         &(lp->projection), NULL, OWS_WMS);

           msOWSPrintBoundingBox( stdout,"        ", &(ext), &(lp->projection),
                                  &(lp->metadata), "MO", nVersion );
       }
       else
       {
           if (nVersion >= OWS_1_3_0)
             msOWSPrintEX_GeographicBoundingBox(stdout, "    ", &(ext),
                                                &(map->projection));
           else
              msOWSPrintLatLonBoundingBox(stdout, "        ", &(ext),
                                         &(map->projection), NULL, OWS_WMS);
           msOWSPrintBoundingBox(stdout,"        ", &(ext), &(map->projection),
                                 &(map->web.metadata), "MO", nVersion );
       }
   }
   else
   {
       if (nVersion >= OWS_1_3_0)
         msIO_printf("        <!-- WARNING: Optional Ex_GeographicBoundingBox could not be established for this layer.  Consider setting the EXTENT in the LAYER object, or wms_extent metadata. Also check that your data exists in the DATA statement -->\n");
       else
         msIO_printf("        <!-- WARNING: Optional LatLonBoundingBox could not be established for this layer.  Consider setting the EXTENT in the LAYER object, or wms_extent metadata. Also check that your data exists in the DATA statement -->\n");
   }

   /* time support */
   pszWmsTimeExtent = msOWSLookupMetadata(&(lp->metadata), "MO", "timeextent");
   if (pszWmsTimeExtent)
   {
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

  if (nVersion >= OWS_1_0_7) {
    msWMSPrintAttribution(stdout, "    ", &(lp->metadata), "MO");
  }

   if(nVersion >= OWS_1_1_0)
       msOWSPrintURLType(stdout, &(lp->metadata), "MO", "metadataurl", 
                         OWS_NOERR, NULL, "MetadataURL", " type=\"%s\"", 
                         NULL, NULL, ">\n          <Format>%s</Format", 
                         "\n          <OnlineResource xmlns:xlink=\""
                         "http://www.w3.org/1999/xlink\" "
                         "xlink:type=\"simple\" xlink:href=\"%s\"/>\n        ",
                         MS_TRUE, MS_FALSE, MS_FALSE, MS_TRUE, MS_TRUE, 
                         NULL, NULL, NULL, NULL, NULL, "        ");

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
   /* a Title and a name. We can get those in wms_style. */
   pszStyle = msOWSLookupMetadata(&(lp->metadata), "MO", "style");
   if (pszStyle)
   {
       pszMetadataName = (char*)malloc(strlen(pszStyle)+205);
       sprintf(pszMetadataName, "style_%s_legendurl_href", pszStyle);
       pszLegendURL = msOWSLookupMetadata(&(lp->metadata), "MO", pszMetadataName);
   }
   else
     pszStyle = "default";

       
   if(nVersion <= OWS_1_0_0 && pszLegendURL)
   {
       /* First, print the style block */
       msIO_fprintf(stdout, "        <Style>\n");
       msIO_fprintf(stdout, "          <Name>%s</Name>\n", pszStyle);
       msIO_fprintf(stdout, "          <Title>%s</Title>\n", pszStyle);

          
       /* Inside, print the legend url block */
       msOWSPrintEncodeMetadata(stdout, &(lp->metadata), "MO", 
                                pszMetadataName,
                                OWS_NOERR, 
                                "          <StyleURL>%s</StyleURL>\n", 
                                NULL);

       /* close the style block */
       msIO_fprintf(stdout, "        </Style>\n");

   }
   else if(nVersion >= OWS_1_1_0)
   {
       if (pszLegendURL)
       {
           /* First, print the style block */
           msIO_fprintf(stdout, "        <Style>\n");
           msIO_fprintf(stdout, "          <Name>%s</Name>\n", pszStyle);
           msIO_fprintf(stdout, "          <Title>%s</Title>\n", pszStyle);

           
           /* Inside, print the legend url block */
           sprintf(pszMetadataName, "style_%s_legendurl", pszStyle);
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
               
       }
       else
       {
           if (script_url_encoded)
           {
               if (lp->connectiontype != MS_WMS && 
                   lp->connectiontype != MS_WFS &&
                   lp->connectiontype != MS_UNUSED_1 &&
                   lp->numclasses > 0)
               {
                   char width[10], height[10];
                   char *legendurl = NULL;
                   int classnameset = 0, i=0;
                   for (i=0; i<lp->numclasses; i++)
                   {
                       if (lp->class[i]->name && 
                           strlen(lp->class[i]->name) > 0)
                       {
                           classnameset = 1;
                           break;
                       }
                   }
                   if (classnameset)
                   {
                       int size_x=0, size_y=0;
                       if (msLegendCalcSize(map, 1, &size_x, &size_y, lp) == MS_SUCCESS)
                       {
                           sprintf(width,  "%d", size_x);
                           sprintf(height, "%d", size_y);

                           legendurl = (char*)malloc(strlen(script_url_encoded)+200);

#ifdef USE_GD_PNG
                           mimetype = msEncodeHTMLEntities("image/png");
#endif
#ifdef USE_GD_GIF
                           if (!mimetype)
                             mimetype = msEncodeHTMLEntities("image/gif");
#endif

#ifdef USE_GD_JPEG
                           if (!mimetype)
                             mimetype = msEncodeHTMLEntities("image/jpeg");
#endif
#ifdef USE_GD_WBMP
                           if (!mimetype)
                             mimetype = msEncodeHTMLEntities("image/vnd.wap.wbmp");
#endif
                           if (!mimetype)
                               mimetype = msEncodeHTMLEntities(MS_IMAGE_MIME_TYPE(map->outputformat));

                           /* -------------------------------------------------------------------- */
                           /*      check if the group parameters for the classes are set. We       */
                           /*      should then publish the deffrent class groups as diffrent styles.*/
                           /* -------------------------------------------------------------------- */
                           iclassgroups = 0;
                           classgroups = NULL;
                           for (i=0; i<lp->numclasses; i++)
                           {
                               if (lp->class[i]->name && lp->class[i]->group)
                               {
                                   if (!classgroups)
                                   {
                                       classgroups = (char **)malloc(sizeof(char *));
                                       classgroups[iclassgroups++]= strdup(lp->class[i]->group);
                                   }
                                   else
                                   {
                                       for (j=0; j<iclassgroups; j++)
                                       {
                                           if (strcasecmp(classgroups[j], lp->class[i]->group) == 0)
                                             break;
                                       }
                                       if (j == iclassgroups)
                                       {
                                           iclassgroups++;
                                           classgroups = (char **)realloc(classgroups, sizeof(char *)*iclassgroups);
                                           classgroups[iclassgroups-1]= strdup(lp->class[i]->group);
                                       }
                                   }
                               }
                           }
                           if (classgroups == NULL)
                           {
                               classgroups = (char **)malloc(sizeof(char *));
                               classgroups[0]= strdup("default");
                               iclassgroups = 1;
                           }
                           for (i=0; i<iclassgroups; i++)
                           {
                               char *name_encoded = msEncodeHTMLEntities(lp->name);
                               if (nVersion >= OWS_1_3_0)
                                   sprintf(legendurl, "%sversion=%s&amp;service=WMS&amp;request=GetLegendGraphic&amp;sld_version=1.1.0&amp;layer=%s&amp;format=%s&amp;STYLE=%s",  
                                           script_url_encoded,msOWSGetVersionString(nVersion, szVersionBuf),name_encoded,
                                           mimetype,  classgroups[i]);
                               else 
                                   sprintf(legendurl, "%sversion=%s&amp;service=WMS&amp;request=GetLegendGraphic&amp;layer=%s&amp;format=%s&amp;STYLE=%s",  
                                           script_url_encoded,msOWSGetVersionString(nVersion, szVersionBuf),name_encoded,
                                           mimetype,  classgroups[i]);

                               msFree(name_encoded);

                               msIO_fprintf(stdout, "        <Style>\n");
                               msIO_fprintf(stdout, "          <Name>%s</Name>\n",  classgroups[i]);
                               msIO_fprintf(stdout, "          <Title>%s</Title>\n", classgroups[i]);
                      
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
                       

                               msIO_fprintf(stdout, "        </Style>\n");
                           }
                           msFree(legendurl);
                           for (i=0; i<iclassgroups; i++)
                             msFree(classgroups[i]);
                           msFree(classgroups);

                           msFree(mimetype);
                       }
                   }
               }       
           }
       }
   }
   
   msFree(pszMetadataName);

   if (nVersion <  OWS_1_3_0)
     msWMSPrintScaleHint("        ", lp->minscaledenom, lp->maxscaledenom, map->resolution);
   else
     msWMSPrintScaleDenominator("        ", lp->minscaledenom, lp->maxscaledenom);
   msIO_printf("%s    </Layer>\n", indent);

   return MS_SUCCESS;
}

/*
 * msWMSPrepareNestedGroups
 */
void msWMSPrepareNestedGroups(mapObj* map, int nVersion, char*** nestedGroups, int* numNestedGroups)
{
  int i;
  const char* groups;
  char* errorMsg;

  for (i = 0; i < map->numlayers; i++)
  {
    nestedGroups[i] = NULL; /* default */
    numNestedGroups[i] = 0; /* default */
    
    groups = msOWSLookupMetadata(&(GET_LAYER(map, i)->metadata), "MO", "layer_group");
    if ((groups != NULL) && (strlen(groups) != 0))
    {
      if (GET_LAYER(map, i)->group != NULL && strlen(GET_LAYER(map, i)->group) != 0)
      {
        errorMsg = "It is not allowed to set both the GROUP and WMS_LAYER_GROUP for a layer";
        msSetError(MS_WMSERR, errorMsg, "msWMSPrepareNestedGroups()", NULL);
        msIO_fprintf(stdout, "<!-- ERROR: %s -->\n", errorMsg);
        /* cannot return exception at this point because we are already writing to stdout */
      }
      else
      {
        if (groups[0] != '/')
        {      
          errorMsg = "The WMS_LAYER_GROUP metadata does not start with a '/'";
          msSetError(MS_WMSERR, errorMsg, "msWMSPrepareNestedGroups()", NULL);
          msIO_fprintf(stdout, "<!-- ERROR: %s -->\n", errorMsg);
          /* cannot return exception at this point because we are already writing to stdout */
        }
        else
        {
          /* split into subgroups. Start at adres + 1 because the first '/' would cause an extra emtpy group */
          nestedGroups[i] = msStringSplit(groups + 1, '/', &numNestedGroups[i]); 
        }
      }
    }
  }
}

/*
 * msWMSIsSubGroup
 */
int msWMSIsSubGroup(char** currentGroups, int currentLevel, char** otherGroups, int numOtherGroups)
{
   int i;
   /* no match if otherGroups[] has less levels than currentLevel */
   if (numOtherGroups <= currentLevel) 
   {
      return MS_FALSE;
   }
   /* compare all groups below the current level */
   for (i = 0; i <= currentLevel; i++)
   {
      if (strcmp(currentGroups[i], otherGroups[i]) != 0)
      {
         return MS_FALSE; /* if one of these is not equal it is not a sub group */
      }
   }
   return MS_TRUE;
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
	int index, int level, char*** nestedGroups, int* numNestedGroups, const char *script_url_encoded)
{
   int j;

   if (numNestedGroups[index] <= level) /* no more subgroups */
   {
      /* we are at the deepest level of the group branchings, so add layer now. */
      msDumpLayer(map, GET_LAYER(map, index), nVersion, script_url_encoded, "");
      pabLayerProcessed[index] = 1; /* done */
   }
   else /* not yet there, we have to deal with this group and possible subgroups and layers. */
   {
      /* Beginning of a new group... enclose the group in a layer block */
      msIO_printf("    <Layer>\n");
      msIO_printf("    <Title>%s</Title>\n", nestedGroups[index][level]);      

      /* Look for one group deeper in the current layer */
      if (!pabLayerProcessed[index])
      {
         msWMSPrintNestedGroups(map, nVersion, pabLayerProcessed,
                                index, level + 1, nestedGroups, 
                                numNestedGroups, script_url_encoded);
      }

      /* look for subgroups in other layers. */
      for (j = index + 1; j < map->numlayers; j++) 
      {
         if (msWMSIsSubGroup(nestedGroups[index], level, nestedGroups[j], numNestedGroups[j]))
         {
            if (!pabLayerProcessed[j])
            {
               msWMSPrintNestedGroups(map, nVersion, pabLayerProcessed,
                                      j, level + 1, nestedGroups, 
                                      numNestedGroups, script_url_encoded);
            }
         }
         else
         {
            /* TODO: if we would sort all layers on "WMS_LAYER_GROUP" beforehand */
            /* we could break out of this loop at this point, which would increase */
            /* performance.  */
         }
      }
      /* Close group layer block */
      msIO_printf("    </Layer>\n");
   }
   
} /* msWMSPrintNestedGroups */

/*
** msWMSGetCapabilities()
*/
int msWMSGetCapabilities(mapObj *map, int nVersion, cgiRequestObj *req, const char *requested_updatesequence,
                         char *wms_exception_format)
{
  char *dtd_url = NULL;
  char *script_url=NULL, *script_url_encoded=NULL;
  const char *pszMimeType=NULL;
  char szVersionBuf[OWS_VERSION_MAXLEN];
  char *schemalocation = NULL;
  const char *updatesequence=NULL;
  const char *sldenabled=NULL;
  const char *encoding;
  char *pszTmp=NULL;
  int i;

  updatesequence = msOWSLookupMetadata(&(map->web.metadata), "MO", "updatesequence");

  sldenabled = msOWSLookupMetadata(&(map->web.metadata), "MO", "sld_enabled");

  encoding = msOWSLookupMetadata(&(map->web.metadata), "MO", "encoding");

  if (sldenabled == NULL)
      sldenabled = strdup("true");

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
    dtd_url = strdup(schemalocation);
    dtd_url = msStringConcatenate(dtd_url, "/wms/1.0.0/capabilities_1_0_0.dtd");
  }

  else if (nVersion < OWS_1_1_0) {
    nVersion = OWS_1_0_7;
    dtd_url = strdup(schemalocation);
    dtd_url = msStringConcatenate(dtd_url, "/wms/1.0.7/capabilities_1_0_7.dtd");
  }
  else if (nVersion == OWS_1_1_0) {
    nVersion = OWS_1_1_0;
    dtd_url = strdup(schemalocation);
    dtd_url = msStringConcatenate(dtd_url, "/wms/1.1.0/capabilities_1_1_0.dtd");
  }
  /*TODO review wms1.3.0*/
  else if (nVersion != OWS_1_3_0) {
    nVersion = OWS_1_1_1;
    dtd_url = strdup(schemalocation);
    /* this exception was added to accomadote the OGC test suite (Bug 1576)*/
    if (strcasecmp(schemalocation, OWS_DEFAULT_SCHEMAS_LOCATION) == 0)
      dtd_url = msStringConcatenate(dtd_url, "/wms/1.1.1/WMS_MS_Capabilities.dtd");
    else
      dtd_url = msStringConcatenate(dtd_url, "/wms/1.1.1/capabilities_1_1_1.dtd");
  }

  /* We need this server's onlineresource. */
  /* Default to use the value of the "onlineresource" metadata, and if not */
  /* set then build it: "http://$(SERVER_NAME):$(SERVER_PORT)$(SCRIPT_NAME)?" */
  /* the returned string should be freed once we're done with it. */
  if ((script_url=msOWSGetOnlineResource(map, "MO", "onlineresource", req)) == NULL ||
      (script_url_encoded = msEncodeHTMLEntities(script_url)) == NULL)
  {
    return msWMSException(map, nVersion, NULL, wms_exception_format);
  }

  if (nVersion <= OWS_1_0_7 || nVersion >= OWS_1_3_0)   /* 1.0.0 to 1.0.7 and >=1.3.0*/
      if (encoding)
          msIO_printf("Content-type: text/xml; charset=%s%c%c", encoding,10,10);
      else
          msIO_printf("Content-type: text/xml%c%c",10,10);
  else /* 1.1.0 and later */
      if (encoding)
          msIO_printf("Content-type: application/vnd.ogc.wms_xml; charset=%s%c%c", encoding,10,10);
      else
          msIO_printf("Content-type: application/vnd.ogc.wms_xml%c%c",10,10);

  msOWSPrintEncodeMetadata(stdout, &(map->web.metadata),
                           "MO", "encoding", OWS_NOERR,
                "<?xml version='1.0' encoding=\"%s\" standalone=\"no\" ?>\n",
                "ISO-8859-1");

  /*TODO review wms1.3.0*/
  if ( nVersion < OWS_1_3_0)
  {
      msIO_printf("<!DOCTYPE WMT_MS_Capabilities SYSTEM \"%s\"\n", dtd_url);
      msIO_printf(" [\n");

      /* some mapserver specific declarations will go here */
      msIO_printf(" <!ELEMENT VendorSpecificCapabilities EMPTY>\n");

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

  
  if ( nVersion == OWS_1_3_0)
  {
      msIO_printf("  xmlns=\"http://www.opengis.net/wms\""
                  "   xmlns:sld=\"http://www.opengis.net/sld\""
                  "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
                  "   xmlns:ms=\"http://mapserver.gis.umn.edu/mapserver\""
                  "   xsi:schemaLocation=\"http://www.opengis.net/wms %s/wms/%s/capabilities_1_3_0.xsd "
                  " http://www.opengis.net/sld http://schemas.opengis.net/sld/1.1.0/sld_capabilities.xsd "
                  " http://mapserver.gis.umn.edu/mapserver %sservice=WMS&amp;version=1.3.0&amp;request=GetSchemaExtension\"",
         msOWSGetSchemasLocation(map), msOWSGetVersionString(nVersion, szVersionBuf), script_url_encoded);
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
  msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "MO", "title",
                           OWS_WARN, "  <Title>%s</Title>\n", map->name);
  msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "MO", "abstract",
                           OWS_NOERR, "  <Abstract>%s</Abstract>\n", NULL);

  if (nVersion == OWS_1_0_0)
  {
      /* <Keywords> in V 1.0.0 */
      /* The 1.0.0 spec doesn't specify which delimiter to use so let's use spaces */
      msOWSPrintEncodeMetadataList(stdout, &(map->web.metadata),
                                   "MO", "keywordlist",
                                   "        <Keywords>",
                                   "        </Keywords>\n",
                                   "%s ", NULL);
  }
  else
  {
      /* <KeywordList><Keyword> ... in V1.0.6+ */
      msOWSPrintEncodeMetadataList(stdout, &(map->web.metadata),
                                   "MO", "keywordlist",
                                   "        <KeywordList>\n",
                                   "        </KeywordList>\n",
                                   "          <Keyword>%s</Keyword>\n", NULL);
  }

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

  if (nVersion >= OWS_1_3_0)
  {
      /* LayerLimit not supported. No restriction in MapServer at this point*/
      msIO_printf("  <MaxWidth>%d</MaxWidth>\n", map->maxsize);
      msIO_printf("  <MaxHeight>%d</MaxHeight>\n", map->maxsize);
  }

  msIO_printf("</Service>\n\n");

  /* WMS capabilities definitions */
  msIO_printf("<Capability>\n");
  msIO_printf("  <Request>\n");

  if (nVersion <= OWS_1_0_7)
  {
    /* WMS 1.0.0 to 1.0.7 - We don't try to use outputformats list here for now */
    msWMSPrintRequestCap(nVersion, "Map", script_url_encoded, ""
#ifdef USE_GD_GIF
                      "<GIF />"
#endif
#ifdef USE_GD_PNG
                      "<PNG />"
#endif
#ifdef USE_GD_JPEG
                      "<JPEG />"
#endif
#ifdef USE_GD_WBMP
                      "<WBMP />"
#endif
                       "<SVG />"  
                      , NULL);
    msWMSPrintRequestCap(nVersion, "Capabilities", script_url_encoded, "<WMS_XML />", NULL);
    msWMSPrintRequestCap(nVersion, "FeatureInfo", script_url_encoded, "<MIME /><GML.1 />", NULL);
  }
  else
  {
    char *mime_list[20];

    /* WMS 1.1.0 and later */
    /* Note changes to the request names, their ordering, and to the formats */

    if (nVersion >= OWS_1_3_0)
      msWMSPrintRequestCap(nVersion, "GetCapabilities", script_url_encoded,
                           "text/xml",
                           NULL);
    else
      msWMSPrintRequestCap(nVersion, "GetCapabilities", script_url_encoded,
                           "application/vnd.ogc.wms_xml",
                           NULL);

    msGetOutputFormatMimeListWMS(map,mime_list,sizeof(mime_list)/sizeof(char*));
    msWMSPrintRequestCap(nVersion, "GetMap", script_url_encoded,
                    mime_list[0], mime_list[1], mime_list[2], mime_list[3],
                    mime_list[4], mime_list[5], mime_list[6], mime_list[7],
                    mime_list[8], mime_list[9], mime_list[10], mime_list[11],
                    mime_list[12], mime_list[13], mime_list[14], mime_list[15],
                    mime_list[16], mime_list[17], mime_list[18], mime_list[19],
                    NULL );

    pszMimeType = msOWSLookupMetadata(&(map->web.metadata), "MO", 
                                      "feature_info_mime_type");

    if (pszMimeType) {
      if (strcasecmp(pszMimeType, "text/plain") == 0) {
        msWMSPrintRequestCap(nVersion, "GetFeatureInfo", script_url_encoded,
                             pszMimeType,
                             "application/vnd.ogc.gml",
                             NULL);
      }
      else {
        msWMSPrintRequestCap(nVersion, "GetFeatureInfo", script_url_encoded,
                             "text/plain",
                             pszMimeType,
                             "application/vnd.ogc.gml",
                             NULL);
      }
    }
    else {
       msWMSPrintRequestCap(nVersion, "GetFeatureInfo", script_url_encoded,
                       "text/plain",
                       "application/vnd.ogc.gml",
                       NULL);
    }

    if (strcasecmp(sldenabled, "true") == 0) {
        if (nVersion == OWS_1_3_0)
           msWMSPrintRequestCap(nVersion, "sld:DescribeLayer", script_url_encoded, "text/xml", NULL);
        else
           msWMSPrintRequestCap(nVersion, "DescribeLayer", script_url_encoded, "text/xml", NULL);

        msGetOutputFormatMimeListImg(map,mime_list,sizeof(mime_list)/sizeof(char*));

        if (nVersion >= OWS_1_1_1) {
           if (nVersion == OWS_1_3_0)
           {
              msWMSPrintRequestCap(nVersion, "sld:GetLegendGraphic", script_url_encoded,
                    mime_list[0], mime_list[1], mime_list[2], mime_list[3],
                    mime_list[4], mime_list[5], mime_list[6], mime_list[7],
                    mime_list[8], mime_list[9], mime_list[10], mime_list[11],
                    mime_list[12], mime_list[13], mime_list[14], mime_list[15],
                    mime_list[16], mime_list[17], mime_list[18], mime_list[19],
                    NULL );
              msWMSPrintRequestCap(nVersion, "ms:GetStyles", script_url_encoded, "text/xml", NULL);
           }
           else 
           {
              msWMSPrintRequestCap(nVersion, "GetLegendGraphic", script_url_encoded,
                    mime_list[0], mime_list[1], mime_list[2], mime_list[3],
                    mime_list[4], mime_list[5], mime_list[6], mime_list[7],
                    mime_list[8], mime_list[9], mime_list[10], mime_list[11],
                    mime_list[12], mime_list[13], mime_list[14], mime_list[15],
                    mime_list[16], mime_list[17], mime_list[18], mime_list[19],
                    NULL );
              msWMSPrintRequestCap(nVersion, "GetStyles", script_url_encoded, "text/xml", NULL);
           }
       }
    }
  }

  msIO_printf("  </Request>\n");

  msIO_printf("  <Exception>\n");
  if (nVersion <= OWS_1_0_7)
      msIO_printf("    <Format><BLANK /><INIMAGE /><WMS_XML /></Format>\n");
  else if (nVersion <= OWS_1_1_1)
  {
      msIO_printf("    <Format>application/vnd.ogc.se_xml</Format>\n");
      msIO_printf("    <Format>application/vnd.ogc.se_inimage</Format>\n");
      msIO_printf("    <Format>application/vnd.ogc.se_blank</Format>\n");
  }
  else /*>=1.3.0*/
  {
       msIO_printf("    <Format>XML</Format>\n");
       msIO_printf("    <Format>INIMAGE</Format>\n");
       msIO_printf("    <Format>BLANK</Format>\n");
       
  }
  msIO_printf("  </Exception>\n");


  if (nVersion != OWS_1_3_0) 
    msIO_printf("  <VendorSpecificCapabilities />\n"); /* nothing yet */

  /* SLD support */
  if (strcasecmp(sldenabled, "true") == 0)
  {
    if (nVersion >= OWS_1_0_7) {
      if (nVersion >= OWS_1_3_0)
        msIO_printf("  <sld:UserDefinedSymbolization SupportSLD=\"1\" UserLayer=\"0\" UserStyle=\"1\" RemoteWFS=\"0\" InlineFeature=\"0\" RemoteWCS=\"0\"/>\n");
      else
        msIO_printf("  <UserDefinedSymbolization SupportSLD=\"1\" UserLayer=\"0\" UserStyle=\"1\" RemoteWFS=\"0\"/>\n");
    }
  }

  /* Top-level layer with map extents and SRS, encloses all map layers */
  msIO_printf("  <Layer>\n");

  /* Layer Name is optional but title is mandatory. */
  if (map->name && strlen(map->name) > 0 &&
      (msIsXMLTagValid(map->name) == MS_FALSE || isdigit(map->name[0])))
    msIO_fprintf(stdout, "<!-- WARNING: The layer name '%s' might contain spaces or "
                 "invalid characters or may start with a number. This could lead to potential problems. -->\n", 
                 map->name);
  msOWSPrintEncodeParam(stdout, "MAP.NAME", map->name, OWS_NOERR,
                        "    <Name>%s</Name>\n", NULL);

  if (msOWSLookupMetadata(&(map->web.metadata), "MO", "rootlayer_title"))
    msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "MO", "rootlayer_title", OWS_WARN, "    <Title>%s</Title>\n", map->name);

  else 
    msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "MO", "title", OWS_WARN, "    <Title>%s</Title>\n", map->name);

  if (msOWSLookupMetadata(&(map->web.metadata), "MO", "rootlayer_abstract"))
    msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "MO", "rootlayer_abstract", OWS_NOERR, "    <Abstract>%s</Abstract>\n", map->name);
  else
    msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "MO", "abstract", OWS_NOERR, "    <Abstract>%s</Abstract>\n", map->name);


  if (msOWSLookupMetadata(&(map->web.metadata), "MO", "rootlayer_keywordlist"))
    pszTmp = strdup("rootlayer_keywordlist");
   else
    pszTmp = strdup("keywordlist");

  if (nVersion == OWS_1_0_0)
  {
     /* <Keywords> in V 1.0.0 */
     /* The 1.0.0 spec doesn't specify which delimiter to use so let's use spaces */
     msOWSPrintEncodeMetadataList(stdout, &(map->web.metadata), "MO",
                                  pszTmp,
                                  "    <Keywords>",
                                  "    </Keywords>\n",
                                  "%s ", NULL);
  }
  else
  {
     /* <KeywordList><Keyword> ... in V1.0.6+ */
     msOWSPrintEncodeMetadataList(stdout, &(map->web.metadata), "MO",
                                  pszTmp,
                                  "    <KeywordList>\n",
                                  "    </KeywordList>\n",
                                  "     <Keyword>%s</Keyword>\n", NULL);
  }

  msFree(pszTmp);

  /* According to normative comments in the 1.0.7 DTD, the root layer's SRS tag */
  /* is REQUIRED.  It also suggests that we use an empty SRS element if there */
  /* is no common SRS. */
  if (nVersion > OWS_1_1_0)
  {
      /* starting 1.1.1 SRS are given in individual tags */
      if (nVersion >= OWS_1_3_0)
          msOWSPrintEncodeParamList(stdout, "(at least one of) "
                                "MAP.PROJECTION, LAYER.PROJECTION "
                                "or wms_srs metadata", 
                                msOWSGetEPSGProj(&(map->projection), 
                                                 &(map->web.metadata),
                                                 "MO", MS_FALSE),
                                OWS_WARN, ' ', NULL, NULL, 
                                "    <CRS>%s</CRS>\n", "");
      else
        msOWSPrintEncodeParamList(stdout, "(at least one of) "
                                "MAP.PROJECTION, LAYER.PROJECTION "
                                "or wms_srs metadata", 
                                msOWSGetEPSGProj(&(map->projection), 
                                                 &(map->web.metadata),
                                                 "MO", MS_FALSE),
                                OWS_WARN, ' ', NULL, NULL, 
                                "    <SRS>%s</SRS>\n", "");
  }
  else
      /* If map has no proj then every layer MUST have one or produce a warning */
    msOWSPrintEncodeParam(stdout, "MAP.PROJECTION (or wms_srs metadata)",
                            msOWSGetEPSGProj(&(map->projection), 
                                             &(map->web.metadata),
                                             "MO", MS_FALSE),
                            OWS_WARN, "    <SRS>%s</SRS>\n", "");

  if (nVersion >= OWS_1_3_0)
    msOWSPrintEX_GeographicBoundingBox(stdout, "    ", &(map->extent),
                                       &(map->projection));
  else
    msOWSPrintLatLonBoundingBox(stdout, "    ", &(map->extent),
                                &(map->projection), NULL, OWS_WMS);

  msOWSPrintBoundingBox( stdout, "    ", &(map->extent), &(map->projection),
                         &(map->web.metadata), "MO", nVersion);

  if (nVersion >= OWS_1_0_7) {
    msWMSPrintAttribution(stdout, "    ", &(map->web.metadata), "MO");
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
  if (map->numlayers)
  {
     int i, j;
     char *pabLayerProcessed = NULL;
     char ***nestedGroups = NULL; 
     int *numNestedGroups = NULL; 

     /* We'll use this array of booleans to track which layer/group have been */
     /* processed already */
     pabLayerProcessed = (char *)calloc(map->numlayers, sizeof(char*));
     /* This array holds the arrays of groups that have been set through the WMS_LAYER_GROUP metadata */
     nestedGroups = (char***)calloc(map->numlayers, sizeof(char**));
     /* This array holds the number of groups set in WMS_LAYER_GROUP for each layer */
     numNestedGroups = (int*)calloc(map->numlayers, sizeof(int));

     msWMSPrepareNestedGroups(map, nVersion, nestedGroups, numNestedGroups);

     for(i=0; i<map->numlayers; i++)
     {
         layerObj *lp;
         lp = (GET_LAYER(map, i));

         if (pabLayerProcessed[i] || (lp->status == MS_DELETE))
             continue;  /* Layer has already been handled */


         if (numNestedGroups[i] > 0) 
         {
            /* Has nested groups.  */
             msWMSPrintNestedGroups(map, nVersion, pabLayerProcessed, i, 0, 
                                    nestedGroups, numNestedGroups, 
                                    script_url_encoded);
         }
         else if (lp->group == NULL || strlen(lp->group) == 0)
         {
             /* This layer is not part of a group... dump it directly */
             msDumpLayer(map, lp, nVersion, script_url_encoded, "");
             pabLayerProcessed[i] = 1;
         }
         else
         {
             /* Beginning of a new group... enclose the group in a layer block */
             msIO_printf("    <Layer>\n");

             /* Layer Name is optional but title is mandatory. */
             if (lp->group &&  strlen(lp->group) > 0 &&
                 (msIsXMLTagValid(lp->group) == MS_FALSE || isdigit(lp->group[0])))
               msIO_fprintf(stdout, "<!-- WARNING: The layer name '%s' might contain spaces or "
                        "invalid characters or may start with a number. This could lead to potential problems. -->\n", 
                            lp->group);
             msOWSPrintEncodeParam(stdout, "GROUP.NAME", lp->group,
                                   OWS_NOERR, "      <Name>%s</Name>\n", NULL);
             msOWSPrintGroupMetadata(stdout, map, lp->group,
                                     "MO", "GROUP_TITLE", OWS_WARN,
                                     "      <Title>%s</Title>\n", lp->group);
             msOWSPrintGroupMetadata(stdout, map, lp->group,
                                     "MO", "GROUP_ABSTRACT", OWS_NOERR,
                                     "      <Abstract>%s</Abstract>\n", lp->group);

             /* Dump all layers for this group */
             for(j=i; j<map->numlayers; j++)
             {
                 if (!pabLayerProcessed[j] &&
                     GET_LAYER(map, j)->group &&
                     strcmp(lp->group, GET_LAYER(map, j)->group) == 0 )
                 {
                     msDumpLayer(map, (GET_LAYER(map, j)), nVersion, script_url_encoded, "  ");
                     pabLayerProcessed[j] = 1;
                 }
             }

             /* Close group layer block */
             msIO_printf("    </Layer>\n");
         }
     }

     free(pabLayerProcessed);

     /* free the stuff used for nested layers */
     for (i = 0; i < map->numlayers; i++)
     {
       if (numNestedGroups[i] > 0)
       {
         msFreeCharArray(nestedGroups[i], numNestedGroups[i]);
       }
     }
     free(nestedGroups);
     free(numNestedGroups);


  }

  msIO_printf("  </Layer>\n");

  msIO_printf("</Capability>\n");
  if ( nVersion >= OWS_1_3_0)
    msIO_printf("</WMS_Capabilities>\n");
  else
    msIO_printf("</WMT_MS_Capabilities>\n");

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
int msTranslateWMS2Mapserv(char **names, char **values, int *numentries)
{
   int i=0;
   int tmpNumentries = *numentries;;

   for (i=0; i<*numentries; i++)
   {
      if (strcasecmp("X", names[i]) == 0)
      {
         values[tmpNumentries] = strdup(values[i]);
         names[tmpNumentries] = strdup("img.x");

         tmpNumentries++;
      }
      else
      if (strcasecmp("Y", names[i]) == 0)
      {
         values[tmpNumentries] = strdup(values[i]);
         names[tmpNumentries] = strdup("img.y");

         tmpNumentries++;
      }
      else
      if (strcasecmp("LAYERS", names[i]) == 0)
      {
         char **layers;
         int tok;
         int j;

         layers = msStringSplit(values[i], ',', &tok);

         for (j=0; j<tok; j++)
         {
            values[tmpNumentries] = layers[j];
            layers[j] = NULL;
            names[tmpNumentries] = strdup("layer");

            tmpNumentries++;
         }

         free(layers);
      }
      else
      if (strcasecmp("QUERY_LAYERS", names[i]) == 0)
      {
         char **layers;
         int tok;
         int j;

         layers = msStringSplit(values[i], ',', &tok);

         for (j=0; j<tok; j++)
         {
            values[tmpNumentries] = layers[j];
            layers[j] = NULL;
            names[tmpNumentries] = strdup("qlayer");

            tmpNumentries++;
         }

         free(layers);
      }
      else
      if (strcasecmp("BBOX", names[i]) == 0)
      {
         char *imgext;

         /* Note msReplaceSubstring() works on the string itself, so we need to make a copy */
         imgext = strdup(values[i]);
         imgext = msReplaceSubstring(imgext, ",", " ");

         values[tmpNumentries] = imgext;
         names[tmpNumentries] = strdup("imgext");

         tmpNumentries++;
      }
   }

   *numentries = tmpNumentries;

   return MS_SUCCESS;
}

/*
** msWMSGetMap()
*/
int msWMSGetMap(mapObj *map, int nVersion, char **names, char **values, int numentries,
                char *wms_exception_format)
{
  imageObj *img;
  int i = 0;
  int sldrequested = MS_FALSE,  sldspatialfilter = MS_FALSE;
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

  for (i=0; i<numentries; i++)
  {
    if ((strcasecmp(names[i], "SLD") == 0 && values[i] && strlen(values[i]) > 0) ||
        (strcasecmp(names[i], "SLD_BODY") == 0 && values[i] && strlen(values[i]) > 0))
    {
        sldrequested = MS_TRUE;
        break;
    }
  }
  if (sldrequested)
  {
      for (i=0; i<map->numlayers; i++)
      {
          if (msLookupHashTable(&(GET_LAYER(map, i)->metadata), "tmp_wms_sld_query"))
          {
              sldspatialfilter = MS_TRUE;
              break;
          }
      }
  }

  if (sldrequested && sldspatialfilter)
  {
      /* set the quermap style so that only selected features will be retruned */
      map->querymap.status = MS_ON;
      map->querymap.style = MS_SELECTED;

      img = msPrepareImage(map, MS_TRUE);
      
      /* compute layer scale factors now */
      for(i=0;i<map->numlayers; i++) {
          if(GET_LAYER(map, i)->sizeunits != MS_PIXELS)
            GET_LAYER(map, i)->scalefactor = (msInchesPerUnit(GET_LAYER(map, i)->sizeunits,0)/msInchesPerUnit(map->units,0)) / map->cellsize;
          else if(GET_LAYER(map, i)->symbolscaledenom > 0 && map->scaledenom > 0)
            GET_LAYER(map, i)->scalefactor = GET_LAYER(map, i)->symbolscaledenom/map->scaledenom;
          else
            GET_LAYER(map, i)->scalefactor = 1;
      }
      for (i=0; i<map->numlayers; i++)
      {
          if (msLookupHashTable(&(GET_LAYER(map, i)->metadata), "tmp_wms_sld_query") &&
              (GET_LAYER(map, i)->type == MS_LAYER_POINT || 
               GET_LAYER(map, i)->type == MS_LAYER_LINE ||
               GET_LAYER(map, i)->type == MS_LAYER_POLYGON ||
               GET_LAYER(map, i)->type == MS_LAYER_ANNOTATION ||
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

  }
  else
    img = msDrawMap(map, MS_FALSE);
  if (img == NULL)
    return msWMSException(map, nVersion, NULL, wms_exception_format);
  
  /* Set the HTTP Cache-control headers if they are defined
     in the map object */
  
  if( (http_max_age = msOWSLookupMetadata(&(map->web.metadata), "MO", "http_max_age")) ) {
    msIO_printf("Cache-Control: max-age=%s\n", http_max_age , 10, 10);
  }
  
  msIO_printf("Content-type: %s%c%c",
              MS_IMAGE_MIME_TYPE(map->outputformat), 10,10);
  if (msSaveImage(map, img, NULL) != MS_SUCCESS)
    return msWMSException(map, nVersion, NULL, wms_exception_format);


  msFreeImage(img);

  return(MS_SUCCESS);
}

int msDumpResult(mapObj *map, int bFormatHtml, int nVersion, char *wms_exception_format)
{
   int numresults=0;
   int i;

   for(i=0; i<map->numlayers; i++)
   {
      int j, k, *itemvisible;
      char **incitems=NULL;
      int numincitems=0;
      char **excitems=NULL;
      int numexcitems=0;
      const char *value;

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

      itemvisible = (int*)malloc(lp->numitems*sizeof(int));
      for(k=0; k<lp->numitems; k++)
      {
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
        if (msLayerResultsGetShape(lp, &shape, lp->resultcache->results[j].tileindex, lp->resultcache->results[j].shapeindex) != MS_SUCCESS)
        {
            msFree(itemvisible);
            return msWMSException(map, nVersion, NULL, wms_exception_format);
        }

        msIO_printf("  Feature %ld: \n", lp->resultcache->results[j].shapeindex);

        for(k=0; k<lp->numitems; k++)
        {
            if (itemvisible[k])
                msIO_printf("    %s = '%s'\n", lp->items[k], shape.values[k]);
        }

        msFreeShape(&shape);
        numresults++;
      }

      msFree(itemvisible);

      /* msLayerClose(lp); */
    }

   return numresults;
}


/*
** msWMSFeatureInfo()
*/
int msWMSFeatureInfo(mapObj *map, int nVersion, char **names, char **values, int numentries,
                     char *wms_exception_format)
{
  int i, feature_count=1, numlayers_found=0;
  pointObj point = {-1.0, -1.0};
  const char *info_format="MIME";
  double cellx, celly;
  errorObj *ms_error = msGetErrorObj();
  int query_status=MS_NOERR;
  const char *pszMimeType=NULL;
  const char *encoding;
  int query_layer = 0;


  pszMimeType = msOWSLookupMetadata(&(map->web.metadata), "MO", "FEATURE_INFO_MIME_TYPE");

  encoding = msOWSLookupMetadata(&(map->web.metadata), "MO", "encoding");

  for(i=0; map && i<numentries; i++) {
    if(strcasecmp(names[i], "QUERY_LAYERS") == 0) {
      char **layers;
      int numlayers, j, k;

      query_layer = 1; /* flag set if QUERY_LAYERS is the request */

      layers = msStringSplit(values[i], ',', &numlayers);
      if(layers==NULL || numlayers < 1 || strlen(msStringTrimLeft(values[i])) < 1) {
        msSetError(MS_WMSERR, "At least one layer name required in QUERY_LAYERS.", "msWMSFeatureInfo()");
        return msWMSException(map, nVersion, "LayerNotDefined", wms_exception_format);
      }

      for(j=0; j<map->numlayers; j++) {
        /* Force all layers OFF by default */
	GET_LAYER(map, j)->status = MS_OFF;

        for(k=0; k<numlayers; k++) {
          if ((GET_LAYER(map, j)->name && strcasecmp(GET_LAYER(map, j)->name, layers[k]) == 0) ||
              (map->name && strcasecmp(map->name, layers[k]) == 0) ||
              (GET_LAYER(map, j)->group && strcasecmp(GET_LAYER(map, j)->group, layers[k]) == 0))
            {
              GET_LAYER(map, j)->status = MS_ON;
              numlayers_found++;
            }
        }
      }

      msFreeCharArray(layers, numlayers);
    } else if (strcasecmp(names[i], "INFO_FORMAT") == 0)
      info_format = values[i];
    else if (strcasecmp(names[i], "FEATURE_COUNT") == 0)
      feature_count = atoi(values[i]);
    else if(strcasecmp(names[i], "X") == 0 || strcasecmp(names[i], "I") == 0)
      point.x = atof(values[i]);
    else if (strcasecmp(names[i], "Y") == 0 || strcasecmp(names[i], "J") == 0)
      point.y = atof(values[i]);
    else if (strcasecmp(names[i], "RADIUS") == 0)
    {
        /* RADIUS in pixels. */
        /* This is not part of the spec, but some servers such as cubeserv */
        /* support it as a vendor-specific feature. */
        /* It's easy for MapServer to handle this so let's do it! */
        int j;
        for(j=0; j<map->numlayers; j++)
        {
            GET_LAYER(map, j)->tolerance = atoi(values[i]);
            GET_LAYER(map, j)->toleranceunits = MS_PIXELS;
        }
    }

  }

  if(numlayers_found == 0) 
  {
      if (query_layer)
      {
          msSetError(MS_WMSERR, "Layer(s) specified in QUERY_LAYERS parameter is not offered by the service instance.", "msWMSFeatureInfo()");
          return msWMSException(map, nVersion, "LayerNotDefined", wms_exception_format);
      }
      else
      {
          msSetError(MS_WMSERR, "Required QUERY_LAYERS parameter missing for getFeatureInfo.", "msWMSFeatureInfo()");
          return msWMSException(map, nVersion, "LayerNotDefined", wms_exception_format);
      }
  }

/* -------------------------------------------------------------------- */
/*      check if all layers selected are queryable. If not send an      */
/*      exception.                                                      */
/* -------------------------------------------------------------------- */
  for (i=0; i<map->numlayers; i++)
  {
      if (GET_LAYER(map, i)->status == MS_ON && !msIsLayerQueryable(GET_LAYER(map, i)))
      {
          msSetError(MS_WMSERR, "Requested layer(s) are not queryable.", "msWMSFeatureInfo()");
          return msWMSException(map, nVersion, "LayerNotQueryable", wms_exception_format);
      }
  }
  if(point.x == -1.0 || point.y == -1.0) {
    if (nVersion >= OWS_1_3_0)
      msSetError(MS_WMSERR, "Required I/J parameters missing for getFeatureInfo.", "msWMSFeatureInfo()");
    else
      msSetError(MS_WMSERR, "Required X/Y parameters missing for getFeatureInfo.", "msWMSFeatureInfo()");
    return msWMSException(map, nVersion, NULL, wms_exception_format);
  }

  /*wms1.3.0: check if the points are valid*/
  if (nVersion >= OWS_1_3_0)
  {
      if (point.x > map->width || point.y > map->height)
      {
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
  map->query.mode = (feature_count==1?MS_SINGLE:MS_MULTIPLE);
  map->query.layer = -1;
  map->query.point = point;
  map->query.buffer = 0;
  map->query.maxresults = feature_count; 

  if(msQueryByPoint(map) != MS_SUCCESS)
    if((query_status=ms_error->code) != MS_NOTFOUND) return msWMSException(map, nVersion, NULL, wms_exception_format);

  /* Generate response */
  if (strcasecmp(info_format, "MIME") == 0 ||
      strcasecmp(info_format, "text/plain") == 0) {

    /* MIME response... we're free to use any valid MIME type */
    int numresults = 0;

    if (encoding)
        msIO_printf("Content-type: text/plain; charset=%s%c%c", encoding,10,10);
    else
        msIO_printf("Content-type: text/plain%c%c",10,10);
    msIO_printf("GetFeatureInfo results:\n");

    numresults = msDumpResult(map, 0, nVersion, wms_exception_format);

    if (numresults == 0) msIO_printf("\n  Search returned no results.\n");

  } else if (strncasecmp(info_format, "GML", 3) == 0 ||  /* accept GML.1 or GML */
             strcasecmp(info_format, "application/vnd.ogc.gml") == 0) {

    if (nVersion <= OWS_1_0_7)
        if (encoding)
            msIO_printf("Content-type: text/xml; charset=%s%c%c", encoding,10,10);
        else
            msIO_printf("Content-type: text/xml%c%c",10,10);
    else
        if (encoding)
            msIO_printf("Content-type: application/vnd.ogc.gml; charset=%s%c%c", encoding,10,10);
        else
            msIO_printf("Content-type: application/vnd.ogc.gml%c%c",10,10);

    msGMLWriteQuery(map, NULL, "GMO"); /* default is stdout */

  } else
  if (pszMimeType && (strcmp(pszMimeType, info_format) == 0))
  {
     mapservObj *msObj;

     msObj = msAllocMapServObj();

     /* Translate some vars from WMS to mapserv */
     msTranslateWMS2Mapserv(names, values, &numentries);

     msObj->map = map;
     msObj->request->ParamNames = names;
     msObj->request->ParamValues = values;
     msObj->Mode = QUERY;
     msObj->request->NumParams = numentries;
     msObj->mappnt.x = point.x;
     msObj->mappnt.y = point.y;

     if (query_status == MS_NOTFOUND && msObj->map->web.empty)
     {
         if(msReturnURL(msObj, msObj->map->web.empty, BROWSE) != MS_SUCCESS)
           return msWMSException(map, nVersion, NULL, wms_exception_format);
     }
     else if (msReturnTemplateQuery(msObj, (char*)pszMimeType, NULL) != MS_SUCCESS)
       return msWMSException(map, nVersion, NULL, wms_exception_format);

     /* We don't want to free the map, and param names/values since they */
     /* belong to the caller, set them to NULL before freeing the mapservObj */
     msObj->map = NULL;
     msObj->request->ParamNames = NULL;
     msObj->request->ParamValues = NULL;
     msObj->request->NumParams = 0;

     msFreeMapServObj(msObj);
  }
  else
  {
     msSetError(MS_WMSERR, "Unsupported INFO_FORMAT value (%s).", "msWMSFeatureInfo()", info_format);
     if (nVersion >= OWS_1_3_0)
       return msWMSException(map, nVersion, "InvalidFormat", wms_exception_format);
     else
       return msWMSException(map, nVersion, NULL, wms_exception_format); 
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
  const char *encoding;

  encoding = msOWSLookupMetadata(&(map->web.metadata), "MO", "encoding");

  for(i=0; map && i<numentries; i++) {
      if(strcasecmp(names[i], "LAYERS") == 0) {
          layers = msStringSplit(values[i], ',', &numlayers);
      }
      if(strcasecmp(names[i], "VERSION") == 0) {
          version = values[i];
      }
      if(strcasecmp(names[i], "SLD_VERSION") == 0) {
          sld_version = values[i];
      }
  }

  if (nVersion >= OWS_1_3_0 && sld_version == NULL)
  {
      msSetError(MS_WMSERR, "Missing required parameter SLD_VERSION", "DescribeLayer()");
      return msWMSException(map, nVersion, "MissingParameterValue", wms_exception_format);
  }
  if (nVersion >= OWS_1_3_0 && strcasecmp(sld_version, "1.1.0") != 0)
  {
      msSetError(MS_WMSERR, "SLD_VERSION must be 1.1.0", "DescribeLayer()");
      return msWMSException(map, nVersion, "InvalidParameterValue", wms_exception_format);
  }

  if (encoding)
      msIO_printf("Content-type: text/xml; charset=%s%c%c", encoding,10,10);
  else
      msIO_printf("Content-type: text/xml%c%c",10,10);

  msOWSPrintEncodeMetadata(stdout, &(map->web.metadata),
                            "MO", "encoding", OWS_NOERR,
                            "<?xml version='1.0' encoding=\"%s\"?>\n",
                            "ISO-8859-1");

   schemalocation = msEncodeHTMLEntities(msOWSGetSchemasLocation(map));
   if (nVersion < OWS_1_3_0)
   {
      
       msIO_printf("<!DOCTYPE WMS_DescribeLayerResponse SYSTEM \"%s/wms/1.1.1/WMS_DescribeLayerResponse.dtd\">\n", schemalocation);
      

       msIO_printf("<WMS_DescribeLayerResponse version=\"%s\" >\n", version);
   }
   else
   {
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

   for(j=0; j<numlayers; j++)
   {
       for(k=0; k<map->numlayers; k++)
       {
         lp = GET_LAYER(map, k);
         if (lp->name && strcasecmp(lp->name, layers[j]) == 0)
         {
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
                                      lp->type == MS_LAYER_POLYGON) )
           {
             pszOnlineResEncoded = msEncodeHTMLEntities(pszOnlineResLyrWFS);
             pszLayerName = msEncodeHTMLEntities(lp->name);

             if (nVersion < OWS_1_3_0)
             {
                 msIO_printf("<LayerDescription name=\"%s\" wfs=\"%s\" owsType=\"WFS\" owsURL=\"%s\">\n",
                             pszLayerName, pszOnlineResEncoded, pszOnlineResEncoded);
                 msIO_printf("<Query typeName=\"%s\" />\n", pszLayerName);
                 msIO_printf("</LayerDescription>\n");
             }
             else /*wms 1.3.0*/
             {
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
           }
           else if (pszOnlineResLyrWCS && lp->type == MS_LAYER_RASTER &&
                    lp->connectiontype != MS_WMS)
           {
               pszOnlineResEncoded = msEncodeHTMLEntities(pszOnlineResLyrWCS);
               pszLayerName = msEncodeHTMLEntities(lp->name);

               if (nVersion < OWS_1_3_0)
               {
                   msIO_printf("<LayerDescription name=\"%s\"  owsType=\"WCS\" owsURL=\"%s\">\n",
                               pszLayerName, pszOnlineResEncoded);
                   msIO_printf("<Query typeName=\"%s\" />\n", pszLayerName);
                   msIO_printf("</LayerDescription>\n");

                   msFree(pszOnlineResEncoded);
                   msFree(pszLayerName);
               }        
               else
               {
                   msIO_printf("  <LayerDescription>\n");
                   msIO_printf("    <owsType>wcs</owsType>\n");
                   msIO_printf("    <se:OnlineResource xlink:type=\"simple\" xlink:href=\"%s\"/>\n", 
                               pszOnlineResEncoded);
                   msIO_printf("    <TypeName>\n");
                   msIO_printf("      <se:CoverageTypeName>%s</se:CoverageTypeName>\n",pszLayerName);
                   msIO_printf("    </TypeName>\n");
                   msIO_printf("  </LayerDescription>\n");
               }
           }
           else
           {
             char *pszLayerName;
             pszLayerName = msEncodeHTMLEntities(lp->name);

             if (nVersion < OWS_1_3_0)
               msIO_printf("<LayerDescription name=\"%s\"></LayerDescription>\n",
                           pszLayerName);
             else /*wms 1.3.0*/
             {
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
           break;
         }
       }
   }

   if (nVersion < OWS_1_3_0)
     msIO_printf("</WMS_DescribeLayerResponse>\n");
   else
      msIO_printf("</DescribeLayerResponse>\n");

   if (layers)
     msFreeCharArray(layers, numlayers);

   return(MS_SUCCESS);
}


/*
** msWMSGetLegendGraphic()
*/
int msWMSGetLegendGraphic(mapObj *map, int nVersion, char **names,
                          char **values, int numentries, char *wms_exception_format)
{
    char *pszLayer = NULL;
    char *pszFormat = NULL;
    char *psRule = NULL;
    char *psScale = NULL;
    int iLayerIndex = -1;
    outputFormatObj *psFormat = NULL;
    imageObj *img=NULL;
    int i = 0;
    int nWidth = -1, nHeight =-1;
    char *pszStyle = NULL;
    char *sld_version = NULL;
    const char *sldenabled = NULL;
    const char *format_list = NULL;
    char **tokens;
    int n,j = 0;
    sldenabled = msOWSLookupMetadata(&(map->web.metadata), "MO", "sld_enabled");

    if (sldenabled == NULL)
       sldenabled = strdup("true");

     for(i=0; map && i<numentries; i++)
     {
         if (strcasecmp(names[i], "LAYER") == 0)
         {
             pszLayer = values[i];
         }
         else if (strcasecmp(names[i], "WIDTH") == 0)
           nWidth = atoi(values[i]);
         else if (strcasecmp(names[i], "HEIGHT") == 0)
           nHeight = atoi(values[i]);
         else if (strcasecmp(names[i], "FORMAT") == 0)
           pszFormat = values[i];
#ifdef USE_OGR
/* -------------------------------------------------------------------- */
/*      SLD support :                                                   */
/*        - check if the SLD parameter is there. it is supposed to      */
/*      refer a valid URL containing an SLD document.                   */
/*        - check the SLD_BODY parameter that should contain the SLD    */
/*      xml string.                                                     */
/* -------------------------------------------------------------------- */
         else if (strcasecmp(names[i], "SLD") == 0 &&
                  values[i] && strlen(values[i]) > 0 && strcasecmp(sldenabled, "true") == 0)
             msSLDApplySLDURL(map, values[i], -1, NULL);
         else if (strcasecmp(names[i], "SLD_BODY") == 0 &&
                  values[i] && strlen(values[i]) > 0 && strcasecmp(sldenabled, "true") == 0)
             msSLDApplySLD(map, values[i], -1, NULL);
         else if (strcasecmp(names[i], "RULE") == 0)
           psRule = values[i];
         else if (strcasecmp(names[i], "SCALE") == 0)
           psScale = values[i];
         else if (strcasecmp(names[i], "STYLE") == 0)
           pszStyle = values[i];
         else if(strcasecmp(names[i], "SLD_VERSION") == 0) 
           sld_version = values[i];
#endif
     }

     if (!pszLayer)
     {
         msSetError(MS_WMSERR, "Mandatory LAYER parameter missing in GetLegendGraphic request.", "msWMSGetLegendGraphic()");
         return msWMSException(map, nVersion, "LayerNotDefined", wms_exception_format);
     }
     if (!pszFormat)
     {
         msSetError(MS_WMSERR, "Mandatory FORMAT parameter missing in GetLegendGraphic request.", "msWMSGetLegendGraphic()");
         return msWMSException(map, nVersion, "InvalidFormat", wms_exception_format);
     }

     if (nVersion >= OWS_1_3_0 && sld_version == NULL)
     {
         msSetError(MS_WMSERR, "Missing required parameter SLD_VERSION", "GetLegendGraphic()");
         return msWMSException(map, nVersion, "MissingParameterValue", wms_exception_format);
     }
     if (nVersion >= OWS_1_3_0 && strcasecmp(sld_version, "1.1.0") != 0)
     {
         msSetError(MS_WMSERR, "SLD_VERSION must be 1.1.0", "GetLegendGraphic()");
         return msWMSException(map, nVersion, "InvalidParameterValue", wms_exception_format);
     }
     /* check if layer name is valid. We only test the layer name and not */
     /* the group name. */
     for (i=0; i<map->numlayers; i++)
     {
         if (GET_LAYER(map, i)->name &&
             strcasecmp(GET_LAYER(map, i)->name, pszLayer) == 0)
         {
             iLayerIndex = i;
             break;
         }
     }

     if (iLayerIndex == -1)
     {
         msSetError(MS_WMSERR, "Invalid layer given in the LAYER parameter.",
                 "msWMSGetLegendGraphic()");
         return msWMSException(map, nVersion, "LayerNotDefined", wms_exception_format);
     }

     /* validate format */
     
     /*check to see if a predefined list is given*/
     format_list = msOWSLookupMetadata(&(map->web.metadata), "M","getlegendgraphic_formatlist");
     n = 0;
     if ( format_list)
       tokens = msStringSplit(format_list,  ',', &n);
      
     if (tokens && n > 0)
     {
         for (j=0; j<n; j++)
         {
             if (strcasecmp(tokens[j], pszFormat) == 0)
               break;
         }
         msFreeCharArray(tokens, n);
         if (j == n || (psFormat = msSelectOutputFormat( map, pszFormat)) == NULL)
         {
             msSetError(MS_IMGERR,
                        "Unsupported output format (%s).",
                        "msWMSGetLegendGraphic()",
                        values[i] );
             return msWMSException(map, nVersion, "InvalidFormat", wms_exception_format);
         }
     }
     else
     {
         psFormat = msSelectOutputFormat( map, pszFormat);
         if( psFormat == NULL || 
             (psFormat->renderer != MS_RENDER_WITH_GD
              &&
              psFormat->renderer != MS_RENDER_WITH_AGG)) 
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

     /*if STYLE is set, check if it is a valid style (valid = at least one
       of the classes have a the group value equals to the style */
     if (pszStyle && strlen(pszStyle) > 0 && strcasecmp(pszStyle, "default") != 0)
     {
         for (i=0; i<GET_LAYER(map, iLayerIndex)->numclasses; i++)
         {
              if (GET_LAYER(map, iLayerIndex)->class[i]->group &&
                  strcasecmp(GET_LAYER(map, iLayerIndex)->class[i]->group, pszStyle) == 0)
                break;
         }
              
         if (i == GET_LAYER(map, iLayerIndex)->numclasses)
         {
             msSetError(MS_WMSERR, "style used in the STYLE parameter is not defined on the layer.", 
                        "msWMSGetLegendGraphic()");
             return msWMSException(map, nVersion, "StyleNotDefined", wms_exception_format);
         }
         else
         {
             if (GET_LAYER(map, iLayerIndex)->classgroup)
               msFree(GET_LAYER(map, iLayerIndex)->classgroup);
             GET_LAYER(map, iLayerIndex)->classgroup = strdup(pszStyle);
             
         }
     }

     if ( psRule == NULL )
     {
         /* turn this layer on and all other layers off, required for msDrawLegend() */
         for (i=0; i<map->numlayers; i++)
         {
             if (i == iLayerIndex)
                 GET_LAYER(map, i)->status = MS_ON;
             else
                 GET_LAYER(map, i)->status = MS_OFF;
         }

         /* if SCALE was provided in request, calculate an extent and use a default width and height */
         if ( psScale != NULL )
         {
             double center_y, scale, cellsize;

             scale = atof(psScale);
             map->width = 600;
             map->height = 600;
             center_y = 0.0;

             cellsize = (scale/map->resolution)/msInchesPerUnit(map->units, center_y);

             map->extent.minx = 0.0 - cellsize*map->width/2.0;
             map->extent.miny = 0.0 - cellsize*map->height/2.0;
             map->extent.maxx = 0.0 + cellsize*map->width/2.0;
             map->extent.maxy = 0.0 + cellsize*map->height/2.0;

             img = msDrawLegend(map, MS_FALSE);
         }
         else
         {
             /* Scale-independent legend */
             img = msDrawLegend(map, MS_TRUE);
         }
     }
     else
     {
         /* RULE was specified. Get the class corresponding to the RULE */
         /* (RULE = class->name) */
         
         for (i=0; i<GET_LAYER(map, iLayerIndex)->numclasses; i++)
         {
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
         if (i < GET_LAYER(map, iLayerIndex)->numclasses)
         {
         /* set the map legend parameters */
             if (nWidth < 0)
             {
                 if (map->legend.keysizex > 0)
                   nWidth = map->legend.keysizex;
                 else
                   nWidth = 20; /* default values : this in not defined in the specs */
             }
             if (nHeight < 0)
             {
                 if (map->legend.keysizey > 0)
                   nHeight = map->legend.keysizey;
                 else
                   nHeight = 20;
             }
             
             img = msCreateLegendIcon(map, GET_LAYER(map, iLayerIndex), 
                                      GET_LAYER(map, iLayerIndex)->class[i],
                                      nWidth, nHeight);
         }
         if (img == NULL)
         {
             msSetError(MS_IMGERR,
                        "Unavailable RULE (%s).",
                        "msWMSGetLegendGraphic()",
                        psRule);
             return msWMSException(map, nVersion, "InvalidRule", wms_exception_format);
         }
     }

     if (img == NULL)
       return msWMSException(map, nVersion, NULL, wms_exception_format);

     msIO_printf("Content-type: %s%c%c", MS_IMAGE_MIME_TYPE(map->outputformat), 10,10);
     if (msSaveImage(map, img, NULL) != MS_SUCCESS)
       return msWMSException(map, nVersion, NULL, wms_exception_format);

     msFreeImage(img);

     return(MS_SUCCESS);
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
    const char *encoding;

    encoding = msOWSLookupMetadata(&(map->web.metadata), "MO", "encoding");

    for(i=0; map && i<numentries; i++)
    {
        /* getMap parameters */
        if (strcasecmp(names[i], "LAYERS") == 0)
        {
            layers = msStringSplit(values[i], ',', &numlayers);
            if (layers==NULL || numlayers < 1) {
                msSetError(MS_WMSERR, "At least one layer name required in LAYERS.",
                   "msWMSGetStyles()");
                return msWMSException(map, nVersion, NULL, wms_exception_format);
            }
            for(j=0; j<map->numlayers; j++)
               GET_LAYER(map, j)->status = MS_OFF;

            for (k=0; k<numlayers; k++)
            {
                for (j=0; j<map->numlayers; j++)
                {
                    if (GET_LAYER(map, j)->name &&
                        strcasecmp(GET_LAYER(map, j)->name, layers[k]) == 0)
                    {
                        GET_LAYER(map, j)->status = MS_ON;
                        validlayer =1;
                    }
                }
            }

            msFreeCharArray(layers, numlayers);
        }

    }

    /* validate all layers given. If an invalid layer is sent, return an exception. */
    if (validlayer == 0)
    {
        msSetError(MS_WMSERR, "Invalid layer(s) given in the LAYERS parameter.",
                   "msWMSGetStyles()");
        return msWMSException(map, nVersion, "LayerNotDefined", wms_exception_format);
    }

    if (nVersion <= OWS_1_1_1)
    {
        if (encoding)
            msIO_printf("Content-type: application/vnd.ogc.sld+xml; charset=%s%c%c", encoding,10,10);
        else
            msIO_printf("Content-type: application/vnd.ogc.sld+xml%c%c",10,10);
        sld = msSLDGenerateSLD(map, -1, "1.0.0");
    }
    else
    {   
      /*for wms 1.3.0 generate a 1.1 sld*/
        if (encoding)
            msIO_printf("Content-type: text/xml; charset=%s%c%c", encoding,10,10);
        else
            msIO_printf("Content-type: text/xml%c%c",10,10);
        sld = msSLDGenerateSLD(map, -1, "1.1.0");
    }
    if (sld)
    {
        msIO_printf("%s\n", sld);
        free(sld);
    }

    return(MS_SUCCESS);
}

int msWMSGetSchemaExtension(mapObj *map) 
{
    char *schemalocation = NULL;
    const char *encoding;

    schemalocation = msEncodeHTMLEntities(msOWSGetSchemasLocation(map));

    encoding = msOWSLookupMetadata(&(map->web.metadata), "MO", "encoding");

    if (encoding)
        msIO_printf("Content-type: text/xml; charset=%s%c%c", encoding,10,10);
    else
        msIO_printf("Content-type: text/xml%c%c",10,10);

    msOWSPrintEncodeMetadata(stdout, &(map->web.metadata),
                            "MO", "encoding", OWS_NOERR,
                            "<?xml version='1.0' encoding=\"%s\"?>\n",
                            "ISO-8859-1");
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
int msWMSDispatch(mapObj *map, cgiRequestObj *req)
{
#ifdef USE_WMS_SVR
  int i, status, nVersion=OWS_VERSION_NOTSET;
  const char *version=NULL, *request=NULL, *service=NULL, *format=NULL, *updatesequence=NULL;
  const char * encoding;
  char *wms_exception_format = NULL;

  encoding = msOWSLookupMetadata(&(map->web.metadata), "MO", "encoding");

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
  }

  /* If SERVICE is specified then it MUST be "WMS" */
  if (service != NULL && strcasecmp(service, "WMS") != 0)
      return MS_DONE;  /* Not a WMS request */

  nVersion = msOWSParseVersionString(version);
  if (nVersion == OWS_VERSION_BADFORMAT)
  {
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
      (nVersion >= OWS_1_0_7 || nVersion == OWS_VERSION_NOTSET))
  {
      msSetError(MS_WMSERR, "Required SERVICE parameter missing.", "msWMSDispatch");
      return msWMSException(map, nVersion, "ServiceNotDefined", wms_exception_format);
  }

  /*
  ** Dispatch request... we should probably do some validation on VERSION here
  ** vs the versions we actually support.
  */
  if (request && (strcasecmp(request, "capabilities") == 0 ||
                  strcasecmp(request, "GetCapabilities") == 0) )
  {
      if (nVersion == OWS_VERSION_NOTSET)
          nVersion = OWS_1_3_0;/* VERSION is optional with getCapabilities only */
      if ((status = msOWSMakeAllLayersUnique(map)) != MS_SUCCESS)
        return msWMSException(map, nVersion, NULL, wms_exception_format);
      return msWMSGetCapabilities(map, nVersion, req, updatesequence, wms_exception_format);
  }
  else if (request && (strcasecmp(request, "context") == 0 ||
                       strcasecmp(request, "GetContext") == 0) )
  {
      /* Return a context document with all layers in this mapfile
       * This is not a standard WMS request.
       * __TODO__ The real implementation should actually return only context
       * info for selected layers in the LAYERS parameter.
       */
      const char *getcontext_enabled;
      getcontext_enabled = msOWSLookupMetadata(&(map->web.metadata),
                                               "MO", "getcontext_enabled");

      if (nVersion != OWS_VERSION_NOTSET)
      {
          /* VERSION, if specified, is Map Context version, not WMS version */
          /* Pass it via wms_context_version metadata */
          char szVersion[OWS_VERSION_MAXLEN];
          msInsertHashTable(&(map->web.metadata), "wms_context_version",
                            msOWSGetVersionString(nVersion, szVersion));
      }
      /* Now set version to 1.1.1 for error handling purposes */
      nVersion = OWS_1_1_1;

      if (getcontext_enabled==NULL || atoi(getcontext_enabled) == 0)
      {
        msSetError(MS_WMSERR, "GetContext not enabled on this server.",
                   "msWMSDispatch()");
        return msWMSException(map, nVersion, NULL, wms_exception_format);
      }

      if ((status = msOWSMakeAllLayersUnique(map)) != MS_SUCCESS)
        return msWMSException(map, nVersion, NULL, wms_exception_format);

      if (encoding)
          msIO_printf("Content-type: text/xml; charset=%s%c%c", encoding,10,10);
      else
          msIO_printf("Content-type: text/xml%c%c",10,10);

      if ( msWriteMapContext(map, stdout) != MS_SUCCESS )
        return msWMSException(map, nVersion, NULL, wms_exception_format);
      /* Request completed */
      return MS_SUCCESS;
  }
  else if (request && strcasecmp(request, "GetMap") == 0 &&
           format && strcasecmp(format,  "image/txt") == 0)
  {
      /* Until someone adds full support for ASCII graphics this should do. ;) */
      if (encoding)
          msIO_printf("Content-type: text/plain; charset=%s%c%c", encoding,10,10);
      else
          msIO_printf("Content-type: text/plain%c%c",10,10);
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
  if (nVersion == OWS_VERSION_NOTSET)
  {
      msSetError(MS_WMSERR,
                 "Incomplete WMS request: VERSION parameter missing",
                 "msWMSDispatch()");
      return msWMSException(map, OWS_VERSION_NOTSET, NULL, wms_exception_format);
  }

  if (request==NULL)
  {
      msSetError(MS_WMSERR,
                 "Incomplete WMS request: REQUEST parameter missing",
                 "msWMSDispatch()");
      return msWMSException(map, nVersion, NULL, wms_exception_format);
  }

  if ((status = msOWSMakeAllLayersUnique(map)) != MS_SUCCESS)
    return msWMSException(map, nVersion, NULL, wms_exception_format);

  if (strcasecmp(request, "GetLegendGraphic") == 0)
    return msWMSGetLegendGraphic(map, nVersion, req->ParamNames, req->ParamValues, req->NumParams,
                                 wms_exception_format);

  if (strcasecmp(request, "GetStyles") == 0)
    return msWMSGetStyles(map, nVersion, req->ParamNames, req->ParamValues, req->NumParams, 
                          wms_exception_format);

  else if (request && strcasecmp(request, "GetSchemaExtension") == 0)
    return msWMSGetSchemaExtension(map);

  /* getMap parameters are used by both getMap and getFeatureInfo */
  if (strcasecmp(request, "map") == 0 || strcasecmp(request, "GetMap") == 0 ||
      strcasecmp(request, "feature_info") == 0 || strcasecmp(request, "GetFeatureInfo") == 0 || strcasecmp(request, "DescribeLayer") == 0)
  {
    status = msWMSLoadGetMapParams(map, nVersion, req->ParamNames, req->ParamValues, req->NumParams,
                                   wms_exception_format);
      if (status != MS_SUCCESS) return status;
  }


  if (strcasecmp(request, "map") == 0 || strcasecmp(request, "GetMap") == 0)
    return msWMSGetMap(map, nVersion, req->ParamNames, req->ParamValues, req->NumParams, 
                       wms_exception_format);
  else if (strcasecmp(request, "feature_info") == 0 || strcasecmp(request, "GetFeatureInfo") == 0)
    return msWMSFeatureInfo(map, nVersion, req->ParamNames, req->ParamValues, req->NumParams,
                            wms_exception_format);
  else if (strcasecmp(request, "DescribeLayer") == 0)
  {
    return msWMSDescribeLayer(map, nVersion, req->ParamNames, req->ParamValues, req->NumParams,
                              wms_exception_format);
  }

  /* Hummmm... incomplete or unsupported WMS request */
  if (service != NULL && strcasecmp(service, "WMS") == 0)
  {
      msSetError(MS_WMSERR, "Incomplete or unsupported WMS request", "msWMSDispatch()");
      return msWMSException(map, nVersion, NULL, wms_exception_format);
  }
  else
    return MS_DONE;  /* Not a WMS request */
#else
  msSetError(MS_WMSERR, "WMS server support is not available.", "msWMSDispatch()");
  return(MS_FAILURE);
#endif
}


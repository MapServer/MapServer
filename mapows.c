/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  OGC Web Services (WMS, WFS) support functions
 * Author:   Daniel Morissette, DM Solutions Group (morissette@dmsolutions.ca)
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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.67  2006/08/23 18:28:00  dan
 * Use OWS_DEFAULT_SCHEMAS_LOCATION #define instead of hardcoded string (bug 1873)
 *
 * Revision 1.66  2006/08/23 17:52:06  dan
 * schemas.opengeospatial.net has been shutdown, use schemas.opengis.net
 * instead as the default schema repository for OGC services (bug 1873)
 *
 * Revision 1.65  2006/06/06 14:21:01  frank
 * ensure msOWSDispatch() always available
 *
 * Revision 1.64  2006/05/09 14:33:41  assefa
 * WFS client/OWS : correct path concatenation logic for temporary gml file
 * created (bug 1770).
 *
 * Revision 1.63  2006/03/15 18:00:22  assefa
 * Use flag SOS_SVR instead of OGC_SOS (Bug 1712).
 *
 * Revision 1.62  2006/03/14 04:08:34  assefa
 * Add disptach call to SOS service.
 *
 * Revision 1.61  2006/02/14 03:38:47  julien
 * Update to MapContext 1.1.0, add dimensions support in context bug 1581
 *
 * Revision 1.60  2005/06/14 16:03:34  dan
 * Updated copyright date to 2005
 *
 * Revision 1.59  2005/05/24 18:52:45  julien
 * Bug 1149: From WMS 1.1.1, SRS are given in individual tags.
 *
 * Revision 1.58  2005/04/12 23:13:20  sean
 * use non static strings for temp values in msOWSGetLayerExtent() and msOWSGetEPSGProj() (bug 1311).
 *
 * Revision 1.57  2005/02/25 21:32:12  frank
 * fix msOWSGetLayerExtent() for bug 1118
 *
 * Revision 1.56  2005/02/18 03:06:46  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
 *
 * Revision 1.55  2004/12/21 15:59:14  dan
 * Do not include port number in online resource if it's http/80 (bug 1075)
 *
 * Revision 1.54  2004/11/28 02:51:02  frank
 * blow an error if any WxS services are enabled without PROJ
 *
 * Revision 1.53  2004/11/25 06:19:05  dan
 * Add trailing "?" or "&" to connection string when required in WFS
 * client layers using GET method (bug 1082)
 *
 * Revision 1.52  2004/11/16 21:57:49  dan
 * Final pass at updating WMS/WFS client/server interfaces to lookup "ows_*"
 * metadata in addition to default "wms_*"/"wfs_*" metadata (bug 568)
 *
 * Revision 1.51  2004/11/15 21:10:38  dan
 * No need to call msLayerOpen/Close before/after msLayerGetExtent() any more
 * (bug 1051)
 *
 * Revision 1.50  2004/11/09 17:59:05  julien
 * Allow use of msOWSPrintURLType with no metadata. (For bug 1011)
 *
 * Revision 1.49  2004/10/29 22:18:54  assefa
 * Use ows_schama_location metadata. The default value if metadata is not found
 * is http://schemas.opengeospatial.net
 *
 * Revision 1.48  2004/10/27 19:18:25  julien
 * msOWSPrintURLType: Encode string before asssigning end buffer. (Bug 944)
 *
 * Revision 1.47  2004/10/26 21:54:00  dan
 * Poor attempt at clarifying the function header docs for msOWSPrintURLType()
 *
 * Revision 1.46  2004/10/26 15:19:00  julien
 * msOWSPrintURLType: use default values inside the given format. (Bug 944)
 *
 * Revision 1.45  2004/10/25 17:30:38  julien
 * Print function for OGC URLs components. msOWSPrintURLType() (Bug 944)
 *
 * Revision 1.44  2004/10/21 04:30:54  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 * Revision 1.43  2004/10/04 17:02:28  frank
 * fixed handling of default_value in msOWSPrintValidateMetadata
 *
 * Revision 1.42  2004/10/01 19:14:47  frank
 * use msIO_ calls
 *
 * Revision 1.41  2004/09/30 13:01:43  julien
 * Fix a typo in contact information encoding
 *
 * Revision 1.40  2004/09/28 20:43:27  frank
 * avoid warnings
 *
 * Revision 1.39  2004/09/27 19:11:15  dan
 * Fixed small leak in msOWSParseVersionString()
 *
 * Revision 1.38  2004/09/25 17:16:31  julien
 * Don't encode XML tag (Bug 897)
 * Don't compile mapgml.c function if not necessary (Bug 896)
 *
 * Revision 1.37  2004/09/23 19:18:10  julien
 * Encode all metadata and parameter printed in an XML document (Bug 802)
 *
 * Revision 1.36  2004/08/03 23:26:24  dan
 * Cleanup OWS version tests in the code, mapwms.c (bug 799)
 *
 * Revision 1.35  2004/08/03 22:12:34  dan
 * Cleanup OWS version tests in the code, started with mapcontext.c (bug 799)
 *
 * Revision 1.34  2004/06/22 20:55:20  sean
 * Towards resolving issue 737 changed hashTableObj to a structure which contains a hashObj **items.  Changed all hash table access functions to operate on the target table by reference.  msFreeHashTable should not be used on the hashTableObj type members of mapserver structures, use msFreeHashItems instead.
 *
 * Revision 1.33  2004/05/14 05:32:44  sdlime
 * An assert(FALSE) in mapows.c was failing at build, changed to MS_FALSE...
 *
 * Revision 1.32  2004/05/12 20:59:48  dan
 * Fixed typo in OWS namespace code in msOWSGetLayerExtent() and added an
 * assert() in msOWSLookupMetadata() to catch that in the future (bug 661)
 *
 * Revision 1.31  2004/05/03 03:45:42  dan
 * Include map= param in default onlineresource of GetCapabilties if it
 * was explicitly set in QUERY_STRING (bug 643)
 *
 * Revision 1.30  2004/04/23 16:17:53  frank
 * avoid const warnings
 *
 * Revision 1.29  2004/04/21 13:02:42  sdlime
 * Updated msOWSPrintMetadataList() so that NULL values for startTag/endTag
 * are valid.
 *
 * Revision 1.28  2004/04/20 05:41:20  sdlime
 * Getting very close to a usable WCS implementation. Still need to add 
 * domain and range set to DescribeCoverage, and need to be able to interpret
 * requests based on them. However, we'll keep it simple for now, operating
 * on bands and some temporal subsetting.
 *
 * Revision 1.26  2004/04/14 07:31:40  dan
 * Removed msOWSGetMetadata(), replaced by msOWSLookupMetadata()
 *
 * Revision 1.25  2004/04/14 05:14:54  dan
 * Added ability to pass a default value to msOWSPrintMetadataList() (bug 616)
 *
 * Revision 1.24  2004/04/14 04:54:30  dan
 * Created msOWSLookupMetadata() and added namespaces lookup in all
 * msOWSPrint*Metadata() functions. Also pass namespaces=NULL everywhere
 * that calls those functions for now to avoid breaking something just
 * before the release. (bug 615, 568)
 *
 * Revision 1.23  2004/03/30 00:04:49  dan
 * Cleaned up changelog
 *
 * Revision 1.22  2004/02/26 18:38:08  frank
 * fixed last change
 *
 * Revision 1.21  2004/02/26 16:13:24  frank
 * Correct msOWSGetLayerExtent() error message.
 *
 * Revision 1.20  2004/02/26 16:08:49  frank
 * Added check for wcs_extent.
 *
 * Revision 1.19  2004/02/24 06:20:37  sdlime
 * Added msOWSGetMetadata() function.
 *
 * Revision 1.18  2004/02/05 04:40:01  sdlime
 * Added WCS to the OWS request broker function. The WCS request handler 
 * just returns MS_DONE for now.
 *
 * Revision 1.17  2003/09/19 21:54:19  assefa
 * Add support fot the Post request.
 *
 * Revision 1.16  2003/07/29 20:24:18  dan
 * Fixed problem with invalid BoundingBox tag in WMS capabilities (bug 34)
 *
 * Revision 1.15  2003/02/05 04:40:10  sdlime
 * Removed shapepath as an argument from msLayerOpen and msSHPOpenFile. The 
 * shapefile opening routine now expects just a filename. So, you must use 
 * msBuildPath or msBuildPath3 to create a full qualified filename. Relatively
 * simple change, but required lots of changes. Demo still works...
 *
 * Revision 1.14  2003/01/10 06:39:06  sdlime
 * Moved msEncodeHTMLEntities() and msDecodeHTMLEntities() from mapows.c to
 * mapstring.c so they can be used a bit more freely.
 *
 * Revision 1.13  2002/12/20 03:43:03  frank
 * ensure this builds without libcurl
 *
 * Revision 1.12  2002/12/19 06:30:59  dan
 * Enable caching WMS/WFS request using tmp filename built from URL
 *
 * Revision 1.11  2002/12/19 05:17:09  dan
 * Report WFS exceptions, and do not fail on WFS requests returning 0 features
 *
 * Revision 1.10  2002/12/18 16:45:49  dan
 * Fixed WFS capabilities to validate against schema
 *
 * Revision 1.9  2002/12/13 00:57:31  dan
 * Modified WFS implementation to behave more as a real vector data source
 *
 * Revision 1.8  2002/11/20 21:22:32  dan
 * Added msOWSGetSchemasLocation() for use by both WFS and WMS Map Context
 *
 * Revision 1.7  2002/11/20 17:17:21  julien
 * Support version 0.1.2 of MapContext
 * Remove warning from tags
 * Encode and decode all url
 *
 * Revision 1.6  2002/11/19 02:27:04  dan
 * Fixed unterminated buffer in msEncodeHTMLEntities()
 *
 * Revision 1.5  2002/11/07 21:16:45  julien
 * Fix warning in ContactInfo
 *
 * Revision 1.4  2002/11/07 15:46:45  julien
 * Print ContactInfo just if necessary
 *
 * Revision 1.3  2002/10/28 20:31:20  dan
 * New support for WMS Map Context (from Julien)
 *
 * Revision 1.2  2002/10/22 20:03:57  julien
 * Add the mapcontext support
 *
 * Revision 1.2  2002/10/04 21:29:41  dan
 * WFS: Added GetCapabilities and basic GetFeature (still some work to do)
 *
 * Revision 1.1  2002/09/03 03:19:51  dan
 * Set the bases for WFS Server support + moved some WMS/WFS stuff to mapows.c
 *
 **********************************************************************/

#include "map.h"

#include <ctype.h> /* isalnum() */
#include <stdarg.h> 
#include <assert.h>

MS_CVSID("$Id$")

/*
** msOWSDispatch() is the entry point for any OWS request (WMS, WFS, ...)
** - If this is a valid request then it is processed and MS_SUCCESS is returned
**   on success, or MS_FAILURE on failure.
** - If this does not appear to be a valid OWS request then MS_DONE
**   is returned and MapServer is expected to process this as a regular
**   MapServer request.
*/
int msOWSDispatch(mapObj *map, cgiRequestObj *request)
{
    int status = MS_DONE;

    if (!request)
      return status;

#ifdef USE_WMS_SVR
    if ((status = msWMSDispatch(map, request)) != MS_DONE )
        return status;
#endif
#ifdef USE_WFS_SVR
    if ((status = msWFSDispatch(map, request)) != MS_DONE )
        return status;
#endif
#ifdef USE_WCS_SVR
    if ((status = msWCSDispatch(map, request)) != MS_DONE )
        return status;
#endif

#ifdef USE_SOS_SVR
    if ((status = msSOSDispatch(map, request)) != MS_DONE )
        return status;
#endif

    return MS_DONE;  /* Not a WMS/WFS request... let MapServer handle it */
}

#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR) || defined(USE_SOS_SVR)

#if !defined(USE_PROJ)
#error "PROJ.4 is required for WMS, WFS and WCS Server Support."
#endif

/*
** msRenameLayer()
*/
static int msRenameLayer(layerObj *lp, int count)
{
    char *newname;
    newname = (char*)malloc((strlen(lp->name)+5)*sizeof(char));
    if (!newname) 
    {
        msSetError(MS_MEMERR, NULL, "msRenameLayer()");
        return MS_FAILURE;
    }
    sprintf(newname, "%s_%2.2d", lp->name, count);
    free(lp->name);
    lp->name = newname;
    
    return MS_SUCCESS;
}

/*
** msOWSMakeAllLayersUnique()
*/
int msOWSMakeAllLayersUnique(mapObj *map)
{
  int i, j;

  /* Make sure all layers in the map file have valid and unique names */
  for(i=0; i<map->numlayers; i++)
  {
      int count=1;
      for(j=i+1; j<map->numlayers; j++)
      {
          if (map->layers[i].name == NULL || map->layers[j].name == NULL)
          {
              msSetError(MS_MISCERR, 
                         "At least one layer is missing a name in map file.", 
                         "msOWSMakeAllLayersUnique()");
              return MS_FAILURE;
          }
          if (strcasecmp(map->layers[i].name, map->layers[j].name) == 0 &&
              msRenameLayer(&(map->layers[j]), ++count) != MS_SUCCESS)
          {
              return MS_FAILURE;
          }
      }

      /* Don't forget to rename the first layer if duplicates were found */
      if (count > 1 && msRenameLayer(&(map->layers[i]), 1) != MS_SUCCESS)
      {
          return MS_FAILURE;
      }
  }
  return MS_SUCCESS;
}


/*
** msOWSTerminateOnlineResource()
**
** Append trailing "?" or "&" to an onlineresource URL if it doesn't have
** one already. The returned string is then ready to append GET parameters
** to it.
**
** Returns a newly allocated string that should be freed by the caller or
** NULL in case of error.
*/
char * msOWSTerminateOnlineResource(const char *src_url)
{
    char *online_resource = NULL;

    if (src_url == NULL) 
        return NULL;

    online_resource = (char*) malloc(strlen(src_url)+2);

    if (online_resource == NULL)
    {
        msSetError(MS_MEMERR, NULL, "msOWSTerminateOnlineResource()");
        return NULL;
    }

    strcpy(online_resource, src_url);

    /* Append trailing '?' or '&' if missing. */
    if (strchr(online_resource, '?') == NULL)
        strcat(online_resource, "?");
    else
    {
        char *c;
        c = online_resource+strlen(online_resource)-1;
        if (*c != '?' && *c != '&')
            strcpy(c+1, "&");
    }

    return online_resource;
}

/*
** msOWSGetOnlineResource()
**
** Return the online resource for this service.  First try to lookup 
** specified metadata, and if not found then try to build the URL ourselves.
**
** Returns a newly allocated string that should be freed by the caller or
** NULL in case of error.
*/
char * msOWSGetOnlineResource(mapObj *map, const char *namespaces, const char *metadata_name, 
                              cgiRequestObj *req)
{
    const char *value;
    char *online_resource = NULL;

    /* We need this script's URL, including hostname. */
    /* Default to use the value of the "onlineresource" metadata, and if not */
    /* set then build it: "http://$(SERVER_NAME):$(SERVER_PORT)$(SCRIPT_NAME)?" */
    /* (+append the map=... param if it was explicitly passed in QUERY_STRING) */
    /*  */
    if ((value = msOWSLookupMetadata(&(map->web.metadata), namespaces, metadata_name))) 
    {
        online_resource = msOWSTerminateOnlineResource(value);
    }
    else 
    {
        const char *hostname, *port, *script, *protocol="http", *mapparam=NULL;
        int mapparam_len = 0;

        hostname = getenv("SERVER_NAME");
        port = getenv("SERVER_PORT");
        script = getenv("SCRIPT_NAME");

        /* HTTPS is set by Apache to "on" in an HTTPS server ... if not set */
        /* then check SERVER_PORT: 443 is the default https port. */
        if ( ((value=getenv("HTTPS")) && strcasecmp(value, "on") == 0) ||
             ((value=getenv("SERVER_PORT")) && atoi(value) == 443) )
        {
            protocol = "https";
        }

        /* If map=.. was explicitly set then we'll include it in onlineresource
         */
        if (req->type == MS_GET_REQUEST)
        {
            int i;
            for(i=0; i<req->NumParams; i++)
            {
                if (strcasecmp(req->ParamNames[i], "map") == 0)
                {
                    mapparam = req->ParamValues[i];
                    mapparam_len = strlen(mapparam)+5; /* +5 for "map="+"&" */
                    break;
                }
            }
        }

        if (hostname && port && script) {
            online_resource = (char*)malloc(sizeof(char)*(strlen(hostname)+strlen(port)+strlen(script)+mapparam_len+10));
            if (online_resource) 
            {
                if ((atoi(port) == 80 && strcmp(protocol, "http") == 0) ||
                    (atoi(port) == 443 && strcmp(protocol, "https") == 0) )
                    sprintf(online_resource, "%s://%s%s?", protocol, hostname, script);
                else
                    sprintf(online_resource, "%s://%s:%s%s?", protocol, hostname, port, script);

                if (mapparam)
                {
                    int baselen;
                    baselen = strlen(online_resource);
                    sprintf(online_resource+baselen, "map=%s&", mapparam);
                }
            }
        }
        else 
        {
            msSetError(MS_CGIERR, "Impossible to establish server URL.  Please set \"%s\" metadata.", "msOWSGetOnlineResource()", metadata_name);
            return NULL;
        }
    }

    if (online_resource == NULL) 
    {
        msSetError(MS_MEMERR, NULL, "msOWSGetOnlineResource()");
        return NULL;
    }

    return online_resource;
}


/* msOWSGetSchemasLocation()
**
** schemas location is the root of the web tree where all WFS-related 
** schemas can be found on this server.  These URLs must exist in order 
** to validate xml.
**
** Use value of "ows_schemas_location" metadata, if not set then
** return ".." as a default
*/
const char *msOWSGetSchemasLocation(mapObj *map)
{
    const char *schemas_location;

    schemas_location = msLookupHashTable(&(map->web.metadata), 
                                         "ows_schemas_location");
    if (schemas_location == NULL)
      schemas_location = OWS_DEFAULT_SCHEMAS_LOCATION;

    return schemas_location;
}

/* msOWSParseVersionString()
**
** Parse a version string in the format "a.b.c" or "a.b" and return an
** integer in the format 0x0a0b0c suitable for regular integer comparisons.
**
** Returns -1 if version could not be parsed.
*/
int msOWSParseVersionString(const char *pszVersion)
{
    char **digits = NULL;
    int numDigits = 0;

    if (pszVersion)
    {
        int nVersion = 0;
        digits = split(pszVersion, '.', &numDigits);
        if (digits == NULL || numDigits < 2 || numDigits > 3)
        {
            msSetError(MS_WMSERR, 
                       "Invalid version (%s). OWS version must be in the "
                       "format 'x.y' or 'x.y.z'",
                       "msOWSParseVersionString()", pszVersion);
            if (digits)
                msFreeCharArray(digits, numDigits);
            return -1;
        }

        nVersion = atoi(digits[0])*0x010000;
        nVersion += atoi(digits[1])*0x0100;
        if (numDigits > 2)
            nVersion += atoi(digits[2]);

        msFreeCharArray(digits, numDigits);

        return nVersion;
    }

    return -1;
}

/* msOWSGetVersionString()
**
** Returns a OWS version string in the format a.b.c from the integer
** version value passed as argument (0x0a0b0c)
**
** Fills in the pszBuffer and returns a reference to it. Recommended buffer
** size is OWS_VERSION_MAXLEN chars.
*/
const char *msOWSGetVersionString(int nVersion, char *pszBuffer)
{

    if (pszBuffer)
        snprintf(pszBuffer, OWS_VERSION_MAXLEN-1, "%d.%d.%d", 
            (nVersion/0x10000)%0x100, (nVersion/0x100)%0x100, nVersion%0x100);

    return pszBuffer;
}


/*
** msOWSLookupMetadata()
**
** Attempts to lookup a given metadata name in multiple OWS namespaces.
**
** 'namespaces' is a string with a letter for each namespace to lookup 
** in the order they should be looked up. e.g. "MO" to lookup wms_ and ows_
** If namespaces is NULL then this function just does a regular metadata
** lookup.
*/
const char *msOWSLookupMetadata(hashTableObj *metadata, 
                                const char *namespaces, const char *name)
{
    const char *value = NULL;

    if (namespaces == NULL)
    {
        value = msLookupHashTable(metadata, (char*)name);
    }
    else
    {
        char buf[100] = "ows_";

        strncpy(buf+4, name, 95);
        buf[99] = '\0';

        while (value == NULL && *namespaces != '\0')
        {
            switch (*namespaces)
            {
              case 'O':         /* ows_... */
                buf[0] = 'o';
                buf[1] = 'w';
                buf[2] = 's';
                break;
              case 'M':         /* wms_... */
                buf[0] = 'w';
                buf[1] = 'm';
                buf[2] = 's';
                break;
              case 'F':         /* wfs_... */
                buf[0] = 'w';
                buf[1] = 'f';
                buf[2] = 's';
                break;
              case 'C':         /* wcs_... */
                buf[0] = 'w';
                buf[1] = 'c';
                buf[2] = 's';
                break;
              case 'G':         /* gml_... */
                buf[0] = 'g';
                buf[1] = 'm';
                buf[2] = 'l';
                break;
                case 'S':         /* sos_... */
                buf[0] = 's';
                buf[1] = 'o';
                buf[2] = 's';
                break;
              default:
                /* We should never get here unless an invalid code (typo) is */
                /* present in the code, but since this happened before... */
                msSetError(MS_WMSERR, 
                           "Unsupported metadata namespace code (%c).",
                           "msOWSLookupMetadata()", *namespaces );
                assert(MS_FALSE);
                return NULL;
            }

            value = msLookupHashTable(metadata, buf);
            namespaces++;
        }
    }

    return value;
}

/*
** msOWSPrintMetadata()
**
** Attempt to output a capability item.  If corresponding metadata is not 
** found then one of a number of predefined actions will be taken. 
** If a default value is provided and metadata is absent then the 
** default will be used.
*/

int msOWSPrintMetadata(FILE *stream, hashTableObj *metadata, 
                       const char *namespaces, const char *name, 
                       int action_if_not_found, const char *format, 
                       const char *default_value) 
{
    const char *value = NULL;
    int status = MS_NOERR;

    if((value = msOWSLookupMetadata(metadata, namespaces, name)) != NULL)
    { 
        msIO_fprintf(stream, format, value);
    }
    else
    {
        if (action_if_not_found == OWS_WARN)
        {
            msIO_fprintf(stream, "<!-- WARNING: Mandatory metadata '%s%s' was missing in this context. -->\n", (namespaces?"..._":""), name);
            status = action_if_not_found;
        }

        if (default_value)
            msIO_fprintf(stream, format, default_value);
    }

    return status;
}


/*
** msOWSPrintEncodeMetadata()
**
** Attempt to output a capability item.  If corresponding metadata is not 
** found then one of a number of predefined actions will be taken. 
** If a default value is provided and metadata is absent then the 
** default will be used.
** Also encode the value with msEncodeHTMLEntities.
*/

int msOWSPrintEncodeMetadata(FILE *stream, hashTableObj *metadata, 
                             const char *namespaces, const char *name, 
                             int action_if_not_found, 
                             const char *format, const char *default_value) 
{
    const char *value;
    char * pszEncodedValue=NULL;
    int status = MS_NOERR;

    if((value = msOWSLookupMetadata(metadata, namespaces, name)))
    {
        pszEncodedValue = msEncodeHTMLEntities(value);
        msIO_fprintf(stream, format, pszEncodedValue);
        free(pszEncodedValue);
    }
    else
    {
        if (action_if_not_found == OWS_WARN)
        {
            msIO_fprintf(stream, "<!-- WARNING: Mandatory metadata '%s%s' was missing in this context. -->\n", (namespaces?"..._":""), name);
            status = action_if_not_found;
        }

        if (default_value)
        {
            pszEncodedValue = msEncodeHTMLEntities(default_value);
            msIO_fprintf(stream, format, default_value);
            free(pszEncodedValue);
        }
    }

    return status;
}


/*
** msOWSGetEncodeMetadata()
**
** Equivalent to msOWSPrintEncodeMetadata. Returns en encoded value of the
** metadata or the default value.
** Caller should free the returned string.
*/
char *msOWSGetEncodeMetadata(hashTableObj *metadata, 
                             const char *namespaces, const char *name, 
                             const char *default_value)
{
    const char *value;
    char * pszEncodedValue=NULL;    
    if((value = msOWSLookupMetadata(metadata, namespaces, name)))
      pszEncodedValue = msEncodeHTMLEntities(value);
    else if (default_value)
      pszEncodedValue = msEncodeHTMLEntities(default_value);

    return pszEncodedValue;
}
      

/*
** msOWSPrintValidateMetadata()
**
** Attempt to output a capability item.  If corresponding metadata is not 
** found then one of a number of predefined actions will be taken. 
** If a default value is provided and metadata is absent then the 
** default will be used.
** Also validate the value with msIsXMLTagValid.
*/

int msOWSPrintValidateMetadata(FILE *stream, hashTableObj *metadata, 
                               const char *namespaces, const char *name, 
                               int action_if_not_found, 
                               const char *format, const char *default_value) 
{
    const char *value;
    int status = MS_NOERR;

    if((value = msOWSLookupMetadata(metadata, namespaces, name)))
    {
        if(msIsXMLTagValid(value) == MS_FALSE)
            msIO_fprintf(stream, "<!-- WARNING: The value '%s' is not valid in a "
                    "XML tag context. -->\n", value);
        msIO_fprintf(stream, format, value);
    }
    else
    {
        if (action_if_not_found == OWS_WARN)
        {
            msIO_fprintf(stream, "<!-- WARNING: Mandatory metadata '%s%s' was missing in this context. -->\n", (namespaces?"..._":""), name);
            status = action_if_not_found;
        }

        if (default_value)
        {
            if(msIsXMLTagValid(default_value) == MS_FALSE)
                msIO_fprintf(stream, "<!-- WARNING: The value '%s' is not valid "
                        "in a XML tag context. -->\n", default_value);
            msIO_fprintf(stream, format, default_value);
        }
    }

    return status;
}

/*
** msOWSPrintGroupMetadata()
**
** Attempt to output a capability item.  If corresponding metadata is not 
** found then one of a number of predefined actions will be taken. 
** If a default value is provided and metadata is absent then the 
** default will be used.
*/

int msOWSPrintGroupMetadata(FILE *stream, mapObj *map, char* pszGroupName, 
                            const char *namespaces, const char *name, 
                            int action_if_not_found, 
                            const char *format, const char *default_value) 
{
    const char *value;
    char *encoded;
    int status = MS_NOERR;
    int i;

    for (i=0; i<map->numlayers; i++)
    {
       if (map->layers[i].group && (strcmp(map->layers[i].group, pszGroupName) == 0) && &(map->layers[i].metadata))
       {
         if((value = msOWSLookupMetadata(&(map->layers[i].metadata), namespaces, name)))
         { 
            encoded = msEncodeHTMLEntities(value);
            msIO_fprintf(stream, format, encoded);
            msFree(encoded);
            return status;
         }
       }
    }

    if (action_if_not_found == OWS_WARN)
    {
       msIO_fprintf(stream, "<!-- WARNING: Mandatory metadata '%s%s' was missing in this context. -->\n", (namespaces?"..._":""), name);
       status = action_if_not_found;
    }

    if (default_value)
    {
       encoded = msEncodeHTMLEntities(default_value);
       msIO_fprintf(stream, format, encoded);
       msFree(encoded);
    }
   
    return status;
}

/* msOWSPrintURLType()
**
** Attempt to output a URL item in capabilties.  If corresponding metadata 
** is not found then one of a number of predefined actions will be taken. 
** Since it's a capability item, five metadata will be used to populate the
** XML elements.
**
** The 'name' argument is the basename of the metadata items relating to this 
** URL type and the suffixes _type, _width, _height, _format and _href will 
** be appended to the name in the metadata search.
** e.g. passing name=metadataurl will result in the following medata entries 
** being used:
**    ows_metadataurl_type
**    ows_metadataurl_format
**    ows_metadataurl_href
**    ... (width and height are unused for metadata)
**
** As for all the msOWSPrint*() functions, the namespace argument specifies 
** which prefix (ows_, wms_, wcs_, etc.) is used for the metadata names above.
**
** Then the final string will be built from 
** the tag_name and the five metadata. The template is:
** <tag_name%type%width%height%format>%href</tag_name>
**
** For example the width format will usually be " width=\"%s\"". 
** An extern format will be "> <Format>%s</Format"
**
** Another template template may be used, but it needs to contains 5 %s, 
** otherwise leave it to NULL. If tag_format is used then you don't need the 
** tag_name and the tabspace.
**
** Note that all values will be HTML-encoded.
**/
int msOWSPrintURLType(FILE *stream, hashTableObj *metadata, 
                      const char *namespaces, const char *name, 
                      int action_if_not_found, const char *tag_format, 
                      const char *tag_name, const char *type_format, 
                      const char *width_format, const char *height_format, 
                      const char *urlfrmt_format, const char *href_format,
                      int type_is_mandatory, int width_is_mandatory, 
                      int height_is_mandatory, int format_is_mandatory, 
                      int href_is_mandatory, const char *default_type, 
                      const char *default_width, const char *default_height, 
                      const char *default_urlfrmt, const char *default_href, 
                      const char *tabspace)
{
    const char *value;
    char *metadata_name;
    char *encoded;
    int status = MS_NOERR;
    char *type=NULL, *width=NULL, *height=NULL, *urlfrmt=NULL, *href=NULL;

    metadata_name = (char*)malloc(strlen(name)*sizeof(char)+10);

    /* Get type */
    if(type_format != NULL)
    {
        sprintf(metadata_name, "%s_type", name);
        value = msOWSLookupMetadata(metadata, namespaces, metadata_name);
        if(value != NULL)
        {
            encoded = msEncodeHTMLEntities(value);
            type = (char*)malloc(strlen(type_format)+strlen(encoded));
            sprintf(type, type_format, encoded);
            msFree(encoded);
        }
    }

    /* Get width */
    if(width_format != NULL)
    {
        sprintf(metadata_name, "%s_width", name);
        value = msOWSLookupMetadata(metadata, namespaces, metadata_name);
        if(value != NULL)
        {
            encoded = msEncodeHTMLEntities(value);
            width = (char*)malloc(strlen(width_format)+strlen(encoded));
            sprintf(width, width_format, encoded);
            msFree(encoded);
        }
    }

    /* Get height */
    if(height_format != NULL)
    {
        sprintf(metadata_name, "%s_height", name);
        value = msOWSLookupMetadata(metadata, namespaces, metadata_name);
        if(value != NULL)
        {
            encoded = msEncodeHTMLEntities(value);
            height = (char*)malloc(strlen(height_format)+strlen(encoded));
            sprintf(height, height_format, encoded);
            msFree(encoded);
        }
    }

    /* Get format */
    if(urlfrmt_format != NULL)
    {
        sprintf(metadata_name, "%s_format", name);
        value = msOWSLookupMetadata(metadata, namespaces, metadata_name);
        if(value != NULL)
        {
            encoded = msEncodeHTMLEntities(value);
            urlfrmt = (char*)malloc(strlen(urlfrmt_format)+strlen(encoded));
            sprintf(urlfrmt, urlfrmt_format, encoded);
            msFree(encoded);
        }
    }

    /* Get href */
    if(href_format != NULL)
    {
        sprintf(metadata_name, "%s_href", name);
        value = msOWSLookupMetadata(metadata, namespaces, metadata_name);
        if(value != NULL)
        {
            encoded = msEncodeHTMLEntities(value);
            href = (char*)malloc(strlen(href_format)+strlen(encoded));
            sprintf(href, href_format, encoded);
            msFree(encoded);
        }
    }

    msFree(metadata_name);

    if(type || width || height || urlfrmt || href || 
       (!metadata && (default_type || default_width || default_height || 
                      default_urlfrmt || default_href)))
    {
        if((!type && type_is_mandatory) || (!width && width_is_mandatory) || 
           (!height && height_is_mandatory) || 
           (!urlfrmt && format_is_mandatory) || (!href && href_is_mandatory))
        {
            msIO_fprintf(stream, "<!-- WARNING: Some mandatory elements for '%s' are missing in this context. -->\n", tag_name);
            if (action_if_not_found == OWS_WARN)
            {
                msIO_fprintf(stream, "<!-- WARNING: Mandatory metadata '%s%s' was missing in this context. -->\n", (namespaces?"..._":""), name);
                status = action_if_not_found;
            }
        }
        else
        {
            if(!type && type_format && default_type)
            {
                type = (char*) malloc(strlen(type_format) + 
                                      strlen(default_type) + 2);
                sprintf(type, type_format, default_type);
            }
            else if(!type)
                type = strdup("");
            if(!width && width_format && default_width)
            {
                width = (char*) malloc(strlen(width_format) + 
                                      strlen(default_width) + 2);
                sprintf(width, width_format, default_width);
            }
            else if(!width)
                width = strdup("");
            if(!height && height_format && default_height)
            {
                height = (char*) malloc(strlen(height_format) + 
                                      strlen(default_height) + 2);
                sprintf(height, height_format, default_height);
            }
            else if(!height)
                height = strdup("");
            if(!urlfrmt && urlfrmt_format && default_urlfrmt)
            {
                urlfrmt = (char*) malloc(strlen(urlfrmt_format) + 
                                      strlen(default_urlfrmt) + 2);
                sprintf(urlfrmt, urlfrmt_format, default_urlfrmt);
            }
            else if(!urlfrmt)
                urlfrmt = strdup("");
            if(!href && href_format && default_href)
            {
                href = (char*) malloc(strlen(href_format) + 
                                      strlen(default_href) + 2);
                sprintf(href, href_format, default_href);
            }
            else if(!href)
                href = strdup("");

            if(tag_format == NULL)
                msIO_fprintf(stream, "%s<%s%s%s%s%s>%s</%s>\n", tabspace, 
                             tag_name, type, width, height, urlfrmt, href, 
                             tag_name);
            else
                msIO_fprintf(stream, tag_format, 
                             type, width, height, urlfrmt, href);
        }

        msFree(type);
        msFree(width);
        msFree(height);
        msFree(urlfrmt);
        msFree(href);
    }
    else
    {
        if (action_if_not_found == OWS_WARN)
        {
            msIO_fprintf(stream, "<!-- WARNING: Mandatory metadata '%s%s' was missing in this context. -->\n", (namespaces?"..._":""), name);
            status = action_if_not_found;
        }
    }

    return status;
}

/* msOWSPrintParam()
**
** Same as printMetadata() but applied to mapfile parameters.
**/
int msOWSPrintParam(FILE *stream, const char *name, const char *value, 
                    int action_if_not_found, const char *format, 
                    const char *default_value) 
{
    int status = MS_NOERR;

    if(value && strlen(value) > 0)
    { 
        msIO_fprintf(stream, format, value);
    }
    else
    {
        if (action_if_not_found == OWS_WARN)
        {
            msIO_fprintf(stream, "<!-- WARNING: Mandatory mapfile parameter '%s' was missing in this context. -->\n", name);
            status = action_if_not_found;
        }

        if (default_value)
            msIO_fprintf(stream, format, default_value);
    }

    return status;
}

/* msOWSPrintEncodeParam()
**
** Same as printEncodeMetadata() but applied to mapfile parameters.
**/
int msOWSPrintEncodeParam(FILE *stream, const char *name, const char *value, 
                          int action_if_not_found, const char *format, 
                          const char *default_value) 
{
    int status = MS_NOERR;
    char *encode;

    if(value && strlen(value) > 0)
    { 
        encode = msEncodeHTMLEntities(value);
        msIO_fprintf(stream, format, encode);
        msFree(encode);
    }
    else
    {
        if (action_if_not_found == OWS_WARN)
        {
            msIO_fprintf(stream, "<!-- WARNING: Mandatory mapfile parameter '%s' was missing in this context. -->\n", name);
            status = action_if_not_found;
        }

        if (default_value)
        {
            encode = msEncodeHTMLEntities(default_value);
            msIO_fprintf(stream, format, encode);
            msFree(encode);
        }
    }

    return status;
}

/* msOWSPrintMetadataList()
**
** Prints comma-separated lists metadata.  (e.g. keywordList)
**/
int msOWSPrintMetadataList(FILE *stream, hashTableObj *metadata, 
                           const char *namespaces, const char *name, 
                           const char *startTag, 
                           const char *endTag, const char *itemFormat,
                           const char *default_value) 
{
    const char *value;
    if((value = msOWSLookupMetadata(metadata, namespaces, name)) ||
       (value = default_value) != NULL ) 
    {
      char **keywords;
      int numkeywords;
      
      keywords = split(value, ',', &numkeywords);
      if(keywords && numkeywords > 0) {
        int kw;
	    if(startTag) msIO_fprintf(stream, "%s", startTag);
	    for(kw=0; kw<numkeywords; kw++) 
            msIO_fprintf(stream, itemFormat, keywords[kw]);
	    if(endTag) msIO_fprintf(stream, "%s", endTag);
	    msFreeCharArray(keywords, numkeywords);
      }
      return MS_TRUE;
    }
    return MS_FALSE;
}

/* msOWSPrintEncodeMetadataList()
**
** Prints comma-separated lists metadata.  (e.g. keywordList)
** This will print HTML encoded values.
**/
int msOWSPrintEncodeMetadataList(FILE *stream, hashTableObj *metadata, 
                                 const char *namespaces, const char *name, 
                                 const char *startTag, 
                                 const char *endTag, const char *itemFormat,
                                 const char *default_value) 
{
    const char *value;
    char *encoded;
    if((value = msOWSLookupMetadata(metadata, namespaces, name)) ||
       (value = default_value) != NULL ) 
    {
      char **keywords;
      int numkeywords;
      
      keywords = split(value, ',', &numkeywords);
      if(keywords && numkeywords > 0) {
        int kw;
	    if(startTag) msIO_fprintf(stream, "%s", startTag);
	    for(kw=0; kw<numkeywords; kw++)
            {
                encoded = msEncodeHTMLEntities(keywords[kw]);
                msIO_fprintf(stream, itemFormat, encoded);
                msFree(encoded);
            }
	    if(endTag) msIO_fprintf(stream, "%s", endTag);
	    msFreeCharArray(keywords, numkeywords);
      }
      return MS_TRUE;
    }
    return MS_FALSE;
}

/* msOWSPrintEncodeParamList()
**
** Same as msOWSPrintEncodeMetadataList() but applied to mapfile parameters.
**/
int msOWSPrintEncodeParamList(FILE *stream, const char *name, 
                              const char *value, int action_if_not_found, 
                              char delimiter, const char *startTag, 
                              const char *endTag, const char *format, 
                              const char *default_value) 
{
    int status = MS_NOERR;
    char *encoded;
    char **items = NULL;
    int numitems = 0, i;

    if(value && strlen(value) > 0)
        items = split(value, delimiter, &numitems);
    else
    {
        if (action_if_not_found == OWS_WARN)
        {
            msIO_fprintf(stream, "<!-- WARNING: Mandatory mapfile parameter '%s' was missing in this context. -->\n", name);
            status = action_if_not_found;
        }

        if (default_value)
            items = split(default_value, delimiter, &numitems);
    }

    if(items && numitems > 0)
    {
        if(startTag) msIO_fprintf(stream, "%s", startTag);
        for(i=0; i<numitems; i++)
        {
            encoded = msEncodeHTMLEntities(items[i]);
            msIO_fprintf(stream, format, encoded);
            msFree(encoded);
        }
        if(endTag) msIO_fprintf(stream, "%s", endTag);
        msFreeCharArray(items, numitems);
    }

    return status;
}

/*
** msOWSPrintLatLonBoundingBox()
**
** Print a LatLonBoundingBox tag for WMS, or LatLongBoundingBox for WFS
** ... yes, the tag name differs between WMS and WFS, yuck!
**
*/
void msOWSPrintLatLonBoundingBox(FILE *stream, const char *tabspace, 
                                 rectObj *extent, projectionObj *srcproj,
                                 int nService)
{
  const char *pszTag = "LatLonBoundingBox";  /* The default for WMS */
  rectObj ll_ext;

  ll_ext = *extent;

  if (srcproj->numargs > 0 && !pj_is_latlong(srcproj->proj)) {
    msProjectRect(srcproj, NULL, &ll_ext);
  }

  if (nService == OWS_WFS)
      pszTag = "LatLongBoundingBox";

  msIO_fprintf(stream, "%s<%s minx=\"%g\" miny=\"%g\" maxx=\"%g\" maxy=\"%g\" />\n", 
         tabspace, pszTag, ll_ext.minx, ll_ext.miny, ll_ext.maxx, ll_ext.maxy);
}

/*
** Emit a bounding box if we can find projection information.
*/
void msOWSPrintBoundingBox(FILE *stream, const char *tabspace, 
                           rectObj *extent, 
                           projectionObj *srcproj,
                           hashTableObj *metadata,
                           const char *namespaces) 
{
    const char	*value, *resx, *resy;
    char *encoded, *encoded_resx, *encoded_resy;

    /* Look for EPSG code in PROJECTION block only.  "wms_srs" metadata cannot be
     * used to establish the native projection of a layer for BoundingBox purposes.
     */
    value = msOWSGetEPSGProj(srcproj, NULL, namespaces, MS_TRUE);
    
    if( value != NULL )
    {
        encoded = msEncodeHTMLEntities(value);
        msIO_fprintf(stream, "%s<BoundingBox SRS=\"%s\"\n"
               "%s            minx=\"%g\" miny=\"%g\" maxx=\"%g\" maxy=\"%g\"",
               tabspace, encoded, 
               tabspace, extent->minx, extent->miny, 
               extent->maxx, extent->maxy);
        msFree(encoded);

        if( (resx = msOWSLookupMetadata( metadata, "MFO", "resx" )) != NULL &&
            (resy = msOWSLookupMetadata( metadata, "MFO", "resy" )) != NULL )
        {
            encoded_resx = msEncodeHTMLEntities(resx);
            encoded_resy = msEncodeHTMLEntities(resy);
            msIO_fprintf( stream, "\n%s            resx=\"%s\" resy=\"%s\"",
                    tabspace, encoded_resx, encoded_resy );
            msFree(encoded_resx);
            msFree(encoded_resy);
        }
 
        msIO_fprintf( stream, " />\n" );
    }

}


/*
** Print the contact information
*/
void msOWSPrintContactInfo( FILE *stream, const char *tabspace, 
                            int nVersion, hashTableObj *metadata, 
                            const char *namespaces )
{
  int bEnableContact = 0;

  /* contact information is a required element in 1.0.7 but the */
  /* sub-elements such as ContactPersonPrimary, etc. are not! */
  /* In 1.1.0, ContactInformation becomes optional. */
  if (nVersion > OWS_1_0_0) 
  {
    if(msOWSLookupMetadata(metadata, namespaces, "contactperson") ||
       msOWSLookupMetadata(metadata, namespaces, "contactorganization")) 
    {
      if(bEnableContact == 0)
      {
          msIO_fprintf(stream, "%s<ContactInformation>\n", tabspace);
          bEnableContact = 1;
      }

      /* ContactPersonPrimary is optional, but when present then all its  */
      /* sub-elements are mandatory */
      msIO_fprintf(stream, "%s  <ContactPersonPrimary>\n", tabspace);

      msOWSPrintEncodeMetadata(stream, metadata, namespaces, "contactperson", 
                  OWS_WARN, "      <ContactPerson>%s</ContactPerson>\n", NULL);
      msOWSPrintEncodeMetadata(stream, metadata, namespaces, "contactorganization", 
             OWS_WARN, "      <ContactOrganization>%s</ContactOrganization>\n",
             NULL);
      msIO_fprintf(stream, "%s  </ContactPersonPrimary>\n", tabspace);
    }

    if(bEnableContact == 0)
    {

        if(msOWSPrintEncodeMetadata(stream, metadata, namespaces, "contactposition",
                           OWS_NOERR, 
     "    <ContactInformation>\n      <ContactPosition>%s</ContactPosition>\n",
                              NULL) != 0)
            bEnableContact = 1;
    }
    else
    {

        msOWSPrintEncodeMetadata(stream, metadata, namespaces, "contactposition", 
                    OWS_NOERR, "      <ContactPosition>%s</ContactPosition>\n",
                           NULL);
    }

    if(msOWSLookupMetadata( metadata, namespaces, "addresstype" ) || 
       msOWSLookupMetadata( metadata, namespaces, "address" ) || 
       msOWSLookupMetadata( metadata, namespaces, "city" ) ||
       msOWSLookupMetadata( metadata, namespaces, "stateorprovince" ) || 
       msOWSLookupMetadata( metadata, namespaces, "postcode" ) ||
       msOWSLookupMetadata( metadata, namespaces, "country" )) 
    {
      if(bEnableContact == 0)
      {
          msIO_fprintf(stream, "%s<ContactInformation>\n", tabspace);
          bEnableContact = 1;
      }

      /* ContactAdress is optional, but when present then all its  */
      /* sub-elements are mandatory */
      msIO_fprintf(stream, "%s  <ContactAddress>\n", tabspace);

      msOWSPrintEncodeMetadata(stream, metadata, namespaces,"addresstype", OWS_WARN,
                              "        <AddressType>%s</AddressType>\n", NULL);
      msOWSPrintEncodeMetadata(stream, metadata, namespaces, "address", OWS_WARN,
                       "        <Address>%s</Address>\n", NULL);
      msOWSPrintEncodeMetadata(stream, metadata, namespaces, "city", OWS_WARN,
                    "        <City>%s</City>\n", NULL);
      msOWSPrintEncodeMetadata(stream, metadata, namespaces, "stateorprovince", 
           OWS_WARN,"        <StateOrProvince>%s</StateOrProvince>\n", NULL);
      msOWSPrintEncodeMetadata(stream, metadata, namespaces, "postcode", OWS_WARN,
                    "        <PostCode>%s</PostCode>\n", NULL);
      msOWSPrintEncodeMetadata(stream, metadata, namespaces, "country", OWS_WARN,
                    "        <Country>%s</Country>\n", NULL);
      msIO_fprintf(stream, "%s  </ContactAddress>\n", tabspace);
    }


    if(bEnableContact == 0)
    {
        if(msOWSPrintEncodeMetadata(stream, metadata, namespaces, 
                                    "contactvoicetelephone", OWS_NOERR,
                                    "    <ContactInformation>\n"
                   "      <ContactVoiceTelephone>%s</ContactVoiceTelephone>\n",
                                    NULL) != 0)
            bEnableContact = 1;
    }
    else
    {
        msOWSPrintEncodeMetadata(stream, metadata, namespaces, 
                                 "contactvoicetelephone", OWS_NOERR,
                   "      <ContactVoiceTelephone>%s</ContactVoiceTelephone>\n",
                           NULL);
    }

    if(bEnableContact == 0)
    {
        if(msOWSPrintEncodeMetadata(stream, metadata,
                           namespaces, "contactfacsimiletelephone", OWS_NOERR,
                              "    <ContactInformation>\n     "
                 "<ContactFacsimileTelephone>%s</ContactFacsimileTelephone>\n",
                                    NULL) != 0)
            bEnableContact = 1;
    }
    else
    {
        msOWSPrintEncodeMetadata(stream, metadata, 
                           namespaces, "contactfacsimiletelephone", OWS_NOERR,
           "      <ContactFacsimileTelephone>%s</ContactFacsimileTelephone>\n",
                                 NULL);
    }

    if(bEnableContact == 0)
    {
        if(msOWSPrintEncodeMetadata(stream, metadata, 
                           namespaces, "contactelectronicmailaddress", OWS_NOERR,
                              "    <ContactInformation>\n     "
           "<ContactElectronicMailAddress>%s</ContactElectronicMailAddress>\n",
                                    NULL) != 0)
            bEnableContact = 1;
    }
    else
    {
        msOWSPrintEncodeMetadata(stream, metadata, 
                           namespaces, "contactelectronicmailaddress", OWS_NOERR,
         "  <ContactElectronicMailAddress>%s</ContactElectronicMailAddress>\n",
                                 NULL);
    }


    if(bEnableContact == 1)
    {
        msIO_fprintf(stream, "%s</ContactInformation>\n", tabspace);
    }
  }
}

/*
** msOWSGetLayerExtent()
**
** Try to establish layer extent, first looking for "ows_extent" metadata, and
** if not found then call msLayerGetExtent() which will lookup the 
** layer->extent member, and if not found will open layer to read extent.
**
*/
int msOWSGetLayerExtent(mapObj *map, layerObj *lp, const char *namespaces, rectObj *ext)
{
  const char *value;

  if ((value = msOWSLookupMetadata(&(lp->metadata), namespaces, "extent")) != NULL)
  {
    char **tokens;
    int n;

    tokens = split(value, ' ', &n);
    if (tokens==NULL || n != 4) {
      msSetError(MS_WMSERR, "Wrong number of arguments for EXTENT metadata.",
                 "msOWSGetLayerExtent()");
      return MS_FAILURE;
    }
    ext->minx = atof(tokens[0]);
    ext->miny = atof(tokens[1]);
    ext->maxx = atof(tokens[2]);
    ext->maxy = atof(tokens[3]);

    msFreeCharArray(tokens, n);
    return MS_SUCCESS;
  }
  else if( lp->type != MS_LAYER_RASTER )
  {
      return msLayerGetExtent(lp, ext);
  }

  return MS_FAILURE;
}


/**********************************************************************
 *                          msOWSExecuteRequests()
 *
 * Execute a number of WFS/WMS HTTP requests in parallel, and then 
 * update layerObj information with the result of the requests.
 **********************************************************************/
int msOWSExecuteRequests(httpRequestObj *pasReqInfo, int numRequests,
                         mapObj *map, int bCheckLocalCache)
{
    int nStatus, iReq;

    /* Execute requests */
#if defined(USE_WMS_LYR) || defined(USE_WFS_LYR)
    nStatus = msHTTPExecuteRequests(pasReqInfo, numRequests, bCheckLocalCache);
#else
    msSetError(MS_WMSERR, "msOWSExecuteRequests() called apparently without libcurl configured, msHTTPExecuteRequests() not available.",
               "msOWSExecuteRequests()");
    return MS_FAILURE;
#endif

    /* Scan list of layers and call the handler for each layer type to */
    /* pass them the request results. */
    for(iReq=0; iReq<numRequests; iReq++)
    {
        if (pasReqInfo[iReq].nLayerId >= 0 && 
            pasReqInfo[iReq].nLayerId < map->numlayers)
        {
            layerObj *lp;

            lp = &(map->layers[pasReqInfo[iReq].nLayerId]);

            if (lp->connectiontype == MS_WFS)
                msWFSUpdateRequestInfo(lp, &(pasReqInfo[iReq]));
        }
    }

    return nStatus;
}

/**********************************************************************
 *                          msOWSProcessException()
 *
 **********************************************************************/
void msOWSProcessException(layerObj *lp, const char *pszFname, 
                           int nErrorCode, const char *pszFuncName)
{
    FILE *fp;

    if ((fp = fopen(pszFname, "r")) != NULL)
    {
        char *pszBuf=NULL;
        int   nBufSize=0;
        char *pszStart, *pszEnd;

        fseek(fp, 0, SEEK_END);
        nBufSize = ftell(fp);
        rewind(fp);
        pszBuf = (char*)malloc((nBufSize+1)*sizeof(char));
        if (pszBuf == NULL)
        {
            msSetError(MS_MEMERR, NULL, "msOWSProcessException()");
            fclose(fp);
            return;
        }

        if ((int) fread(pszBuf, 1, nBufSize, fp) != nBufSize)
        {
            msSetError(MS_IOERR, NULL, "msOWSProcessException()");
            free(pszBuf);
            fclose(fp);
            return;
        }

        pszBuf[nBufSize] = '\0';


        /* OK, got the data in the buffer.  Look for the <Message> tags */
        if ((strstr(pszBuf, "<WFS_Exception>") &&            /* WFS style */
             (pszStart = strstr(pszBuf, "<Message>")) &&
             (pszEnd = strstr(pszStart, "</Message>")) ) ||
            (strstr(pszBuf, "<ServiceExceptionReport>") &&   /* WMS style */
             (pszStart = strstr(pszBuf, "<ServiceException>")) &&
             (pszEnd = strstr(pszStart, "</ServiceException>")) ))
        {
            pszStart = strchr(pszStart, '>')+1;
            *pszEnd = '\0';
            msSetError(nErrorCode, "Got Remote Server Exception for layer %s: %s",
                       pszFuncName, lp->name?lp->name:"(null)", pszStart);
        }
        else
        {
            msSetError(MS_WFSCONNERR, "Unable to parse Remote Server Exception Message for layer %s.",
                       pszFuncName, lp->name?lp->name:"(null)");
        }

        free(pszBuf);
        fclose(fp);
    }
}

/**********************************************************************
 *                          msOWSBuildURLFilename()
 *
 * Build a unique filename for this URL to use in caching remote server 
 * requests.  Slashes and illegal characters will be turned into '_'
 *
 * Returns a newly allocated buffer that should be freed by the caller or
 * NULL in case of error.
 **********************************************************************/
char *msOWSBuildURLFilename(const char *pszPath, const char *pszURL, 
                            const char *pszExt)
{
    char *pszBuf, *pszPtr;
    int  i, nBufLen;


    nBufLen = strlen(pszURL) + strlen(pszExt) +1;
    if (pszPath)
        nBufLen += (strlen(pszPath)+1);

    pszBuf = (char*)malloc((nBufLen+1)*sizeof(char));
    if (pszBuf == NULL)
    {
        msSetError(MS_MEMERR, NULL, "msOWSBuildURLFilename()");
        return NULL;
    }
    pszBuf[0] = '\0';

    if (pszPath)
    {
#ifdef _WIN32
        if (pszPath[strlen(pszPath) -1] != '/' &&
            pszPath[strlen(pszPath) -1] != '\\')
          sprintf(pszBuf, "%s\\", pszPath);
        else
          sprintf(pszBuf, "%s", pszPath);
#else
        if (pszPath[strlen(pszPath) -1] != '/')
          sprintf(pszBuf, "%s/", pszPath);
        else
          sprintf(pszBuf, "%s", pszPath);
#endif
    }

    pszPtr = pszBuf + strlen(pszBuf);

    for(i=0; pszURL[i] != '\0'; i++)
    {
        if (isalnum(pszURL[i]))
            *pszPtr = pszURL[i];
        else
            *pszPtr = '_';
        pszPtr++;
    }
    
    strcpy(pszPtr, pszExt);

    return pszBuf;
}

/*
** msOWSGetEPSGProj()
**
** Extract projection code for this layer/map.
**
** First look for a xxx_srs metadata. If not found then look for an EPSG 
** code in projectionObj, and if not found then return NULL.
**
** If bReturnOnlyFirstOne=TRUE and metadata contains multiple EPSG codes
** then only the first one (which is assumed to be the layer's default
** projection) is returned.
*/
const char *msOWSGetEPSGProj(projectionObj *proj, hashTableObj *metadata, const char *namespaces, int bReturnOnlyFirstOne)
{
  static char epsgCode[20] ="";
  char *value;

  /* metadata value should already be in format "EPSG:n" or "AUTO:..." */
  if (metadata && ((value = (char *) msOWSLookupMetadata(metadata, namespaces, "srs")) != NULL)) {
    
    if (!bReturnOnlyFirstOne) return value;

    /* caller requested only first projection code */
    strncpy(epsgCode, value, 19);
    epsgCode[19] = '\0';
    if ((value=strchr(epsgCode, ' ')) != NULL) *value = '\0';
    
    return epsgCode;
  } else if (proj && proj->numargs > 0 && (value = strstr(proj->args[0], "init=epsg:")) != NULL && strlen(value) < 20) {
    sprintf(epsgCode, "EPSG:%s", value+10);
    return epsgCode;
  } else if (proj && proj->numargs > 0 && strncasecmp(proj->args[0], "AUTO:", 5) == 0 ) {
    return proj->args[0];
  }

  return NULL;
}


/*
** msOWSGetDimensionInfo()
**
** Extract dimension information from a layer's metadata
**
** Before 4.9, only the time dimension was support. With the addition of
** Web Map Context 1.1.0, we need to support every dimension types. 
** This function get the dimension information from special metadata in
** the layer, but can also return default values for the time dimension.
** 
*/
void msOWSGetDimensionInfo(layerObj *layer, const char *pszDimension, 
                           const char **papszDimUserValue, 
                           const char **papszDimUnits, 
                           const char **papszDimDefault, 
                           const char **papszDimNearValue, 
                           const char **papszDimUnitSymbol, 
                           const char **papszDimMultiValue)
{
    char *pszDimensionItem;

    if(pszDimension == NULL || layer == NULL)
        return;

    pszDimensionItem = (char*)malloc(strlen(pszDimension)+50);

    /* units (mandatory in map context) */
    if(papszDimUnits != NULL)
    {
        sprintf(pszDimensionItem, "dimension_%s_units",          pszDimension);
        *papszDimUnits = msOWSLookupMetadata(&(layer->metadata), "MO",
                                           pszDimensionItem);
    }
    /* unitSymbol (mandatory in map context) */
    if(papszDimUnitSymbol != NULL)
    {
        sprintf(pszDimensionItem, "dimension_%s_unitsymbol",     pszDimension);
        *papszDimUnitSymbol = msOWSLookupMetadata(&(layer->metadata), "MO", 
                                                  pszDimensionItem);
    }
    /* userValue (mandatory in map context) */
    if(papszDimUserValue != NULL)
    {
        sprintf(pszDimensionItem, "dimension_%s_uservalue",      pszDimension);
        *papszDimUserValue = msOWSLookupMetadata(&(layer->metadata), "MO", 
                                                 pszDimensionItem);
    }
    /* default */
    if(papszDimDefault != NULL)
    {
        sprintf(pszDimensionItem, "dimension_%s_default",        pszDimension);
        *papszDimDefault = msOWSLookupMetadata(&(layer->metadata), "MO",
                                               pszDimensionItem);
    }
    /* multipleValues */
    if(papszDimMultiValue != NULL)
    {
        sprintf(pszDimensionItem, "dimension_%s_multiplevalues", pszDimension);
        *papszDimMultiValue = msOWSLookupMetadata(&(layer->metadata), "MO", 
                                                  pszDimensionItem);
    }
    /* nearestValue */
    if(papszDimNearValue != NULL)
    {
        sprintf(pszDimensionItem, "dimension_%s_nearestvalue",   pszDimension);
        *papszDimNearValue = msOWSLookupMetadata(&(layer->metadata), "MO", 
                                                 pszDimensionItem);
    }

    /* Use default time value if necessary */
    if(strcasecmp(pszDimension, "time") == 0)
    {
        if(papszDimUserValue != NULL && *papszDimUserValue == NULL)
            *papszDimUserValue = msOWSLookupMetadata(&(layer->metadata), 
                                                   "MO", "time");
        if(papszDimDefault != NULL && *papszDimDefault == NULL)
            *papszDimDefault = msOWSLookupMetadata(&(layer->metadata), 
                                                 "MO", "timedefault");
        if(papszDimUnits != NULL && *papszDimUnits == NULL)
            *papszDimUnits = "ISO8601";
        if(papszDimUnitSymbol != NULL && *papszDimUnitSymbol == NULL)
            *papszDimUnitSymbol = "t";
        if(papszDimNearValue != NULL && *papszDimNearValue == NULL)
            *papszDimNearValue = "0";
    }

    free(pszDimensionItem);

    return;
}

#endif /* USE_WMS_SVR || USE_WFS_SVR  || USE_WCS_SVR */




/**********************************************************************
 * $Id$
 *
 * Name:     mapwfs.c
 * Project:  MapServer
 * Language: C
 * Purpose:  WFS server implementation
 * Author:   Daniel Morissette, DM Solutions Group (morissette@dmsolutions.ca)
 *
 **********************************************************************
 * Copyright (c) 2002, Daniel Morissette, DM Solutions Group Inc
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
 * Revision 1.1  2002/09/03 03:19:51  dan
 * Set the bases for WFS Server support + moved some WMS/WFS stuff to mapows.c
 *
 **********************************************************************/

#include "map.h"

#if defined(USE_WFS_SVR)

/* There is a dependency to GDAL/OGR for the GML driver and MiniXML parser */
#include "cpl_minixml.h"


/*
** msWFSException()
**
** Report current MapServer error in XML exception format.
*/

static int msWFSException(mapObj *map, const char *wmtversion) 
{
    /* In WFS, exceptions are always XML.
    */
    printf("Content-type: text/xml%c%c",10,10);

    printf("<WFS_Exception>\n");
    printf("  <Exception>\n");
    /* Optional <Locator> element currently unused. */
    printf("    <Message>\n");
    msWriteError(stdout);
    printf("    </Message>\n");
    printf("  </Exception>\n");
    printf("</WFS_Exception>\n");

    return MS_FAILURE; // so we can call 'return msWFSException();' anywhere
}


/*
** msWFSGetCapabilities()
*/
int msWFSGetCapabilities(mapObj *map, const char *wmtver) 
{
    printf("Content-type: text/plain%c%c",10,10);
    printf("msWFSGetCapabilities() not implemented yet.\n\n");

    return MS_SUCCESS;
}



/*
** msWFSDescribeFeatureType()
*/
int msWFSDescribeFeatureType(mapObj *map, const char *wmtver, 
                             char **names, char **values, int numentries)
{
    int i;
    const char *pszTypename = NULL;

    for(i=0; map && i<numentries; i++) 
    {
        if(strcasecmp(names[i], "TYPENAME") == 0) 
        {
            pszTypename = values[i];
        } 

    }

    if(pszTypename == NULL) 
    {
        msSetError(MS_WFSERR, 
              "Required TYPENAME parameter missing for DescribeFeatureType.", 
                   "msWFSDescribeFeatureType()");
        return msWFSException(map, wmtver);
    }


    printf("Content-type: text/plain%c%c",10,10);
    printf("msWFSDescribeFeatureType() not implemented yet.\n\n");

    return MS_SUCCESS;
}


/*
** msWFSGetFeature()
*/
int msWFSGetFeature(mapObj *map, const char *wmtver, 
                    char **names, char **values, int numentries)
{
    int i, maxfeatures=-1;
   
    for(i=0; map && i<numentries; i++) 
    {
        if(strcasecmp(names[i], "TYPENAME") == 0) 
        {

        } 
        else if (strcasecmp(names[i], "PROPERTYNAME") == 0)
        {
     
        }
        else if (strcasecmp(names[i], "MAXFEATURES") == 0) 
        {
            maxfeatures = atoi(values[i]);
        }
        else if (strcasecmp(names[i], "FEATUREID") == 0)
        {
     
        }
        else if (strcasecmp(names[i], "FILTER") == 0)
        {
     
        }
        else if (strcasecmp(names[i], "BBOX") == 0)
        {
     
        }

    }

    if(0) 
    {
        msSetError(MS_WFSERR, 
                   "Required ... parameter missing for GetFeature.", 
                   "msWFSGetFeature()");
        return msWFSException(map, wmtver);
    }

    printf("Content-type: text/plain%c%c",10,10);
    printf("msWFSGetFeature() not implemented yet.\n\n");

    return MS_SUCCESS;
}


#endif /* USE_WFS_SVR */


/*
** msWFSDispatch() is the entry point for WFS requests.
** - If this is a valid request then it is processed and MS_SUCCESS is returned
**   on success, or MS_FAILURE on failure.
** - If this does not appear to be a valid WFS request then MS_DONE
**   is returned and MapServer is expected to process this as a regular
**   MapServer request.
*/
int msWFSDispatch(mapObj *map, char **names, char **values, int numentries)
{
#ifdef USE_WFS_SVR
  int i, status;
  static char *wmtver = NULL, *request=NULL, *service=NULL;

  /*
  ** Process Params common to all requests
  */
  for(i=0; i<numentries; i++) {
    if (strcasecmp(names[i], "VERSION") == 0)
      wmtver = values[i];
    else if (strcasecmp(names[i], "REQUEST") == 0)
      request = values[i];
    else if (strcasecmp(names[i], "SERVICE") == 0)
      service = values[i];
  }

  /* If SERVICE is specified then it MUST be "WFS" */
  if (service != NULL && strcasecmp(service, "WFS") != 0)
      return MS_DONE;  /* Not a WFS request */

  /* If SERVICE, VERSION and REQUEST not included than this isn't a WFS req*/
  if (service == NULL && wmtver==NULL && request==NULL)
      return MS_DONE;  /* Not a WFS request */

  /* VERSION *and* REQUEST required by all WFS requests including 
   * GetCapabilities ... we should probably do some validation on VERSION here
   * vs the versions we actually support.
   */
  if (wmtver==NULL)
  {
      msSetError(MS_WFSERR, 
                 "Incomplete WFS request: VERSION parameter missing", 
                 "msWFSDispatch()");
      return msWFSException(map, wmtver);
  }

  if (request==NULL)
  {
      msSetError(MS_WFSERR, 
                 "Incomplete WFS request: REQUEST parameter missing", 
                 "msWFSDispatch()");
      return msWFSException(map, wmtver);
  }

  if ((status = msOWSMakeAllLayersUnique(map)) != MS_SUCCESS)
      return msWFSException(map, wmtver);

  /*
  ** Dispatch request
  */
  if (strcasecmp(request, "GetCapabilities") == 0 ) 
      return msWFSGetCapabilities(map, wmtver);
  else if (strcasecmp(request, "DescribeFeatureType") == 0)
      return msWFSDescribeFeatureType(map, wmtver, names, values, numentries);
  else if (strcasecmp(request, "GetFeature") == 0)
      return msWFSGetFeature(map, wmtver, names, values, numentries);
  else if (strcasecmp(request, "GetFeatureWithLock") == 0 ||
           strcasecmp(request, "LockFeature") == 0 ||
           strcasecmp(request, "Transaction") == 0 )
  {
      // Unsupported WFS request
      msSetError(MS_WFSERR, "Unsupported WFS request", "msWFSDispatch()");
      return msWFSException(map, wmtver);
  }

  // This was not detected as a WFS request... let MapServer handle it
  return MS_DONE;

#else
  msSetError(MS_WFSERR, "WFS server support is not available.", 
             "msWFSDispatch()");
  return(MS_FAILURE);
#endif
}


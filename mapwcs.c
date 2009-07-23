/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  OpenGIS Web Coverage Server (WCS) Implementation.
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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.71.4.1  2007/01/19 03:46:45  sdlime
 * Fixed requestResponseCRSs output to create multiple tags instead of one with a space delimited list. (bug 1148)
 *
 * Revision 1.71  2005/09/23 04:37:32  frank
 * added preliminary WCS INTERPOLATION support
 *
 * Revision 1.70  2005/06/27 19:58:30  frank
 * Fixed some small WCS params related memory leaks.
 *
 * Revision 1.69  2005/06/14 16:03:35  dan
 * Updated copyright date to 2005
 *
 * Revision 1.68  2005/05/12 18:14:52  frank
 * another missing WCS exception - if msSaveImage() fails
 *
 * Revision 1.67  2005/05/12 17:41:49  frank
 * If msDrawRasterLayerLow() fails, ensure that the error message is posted as a WCS exception.
 *
 * Revision 1.66  2005/02/18 18:14:11  dan
 * New patch to fix url strings broken by previous patch to bug 1238
 *
 * Revision 1.65  2005/02/18 16:09:18  hobu
 * patch up msvc errors related to the comment commit
 *
 * Revision 1.64  2005/02/18 03:06:47  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
 *
 * Revision 1.63  2005/02/12 04:41:32  sdlime
 * Changed %g to %.15g for WCS output (bug 1232).
 *
 * Revision 1.62  2004/11/29 14:36:22  sdlime
 * Last of the typos for bug 954, also use ows_service_onlineresource if using WMS metadata.
 *
 * Revision 1.61  2004/11/29 04:30:14  sdlime
 * Fixed typo in ResponsibleParty, positionName tag.
 *
 * Revision 1.60  2004/11/29 04:09:23  sdlime
 * Fixed bug 954, added ResponsibleParty output to the service block. Supports new wcs_responsibleparty_* metadata tags or tries to use WMS contact metadata.
 *
 * Revision 1.59  2004/11/16 21:57:49  dan
 * Final pass at updating WMS/WFS client/server interfaces to lookup "ows_*"
 * metadata in addition to default "wms_*"/"wfs_*" metadata (bug 568)
 *
 * Revision 1.58  2004/11/08 00:33:00  frank
 * Fixed support for non-square pixels in WCS. (bug 1014)
 *
 * Revision 1.57  2004/11/04 21:47:07  frank
 * Removed unused variable.
 *
 * Revision 1.56  2004/10/29 22:48:03  assefa
 * Use of metadata ows_schema_location (Bug 1013).
 *
 * Revision 1.55  2004/10/28 01:27:32  frank
 * Generate an error if a GetCoverage lacks a COVERAGE specification.
 * Must nicer than just segfaulting.
 *
 * Revision 1.54  2004/10/26 15:20:19  julien
 * msOWSPrintURLType: metadata:Link had a %s missing in it's format. (Bug 944)
 *
 * Revision 1.53  2004/10/26 04:26:28  sdlime
 * Fixed bug 989, metadataLink for the CoverageOffering section of the WCS capabilities.
 *
 * Revision 1.52  2004/10/25 17:30:38  julien
 * Print function for OGC URLs components. msOWSPrintURLType() (Bug 944)
 *
 * Revision 1.51  2004/10/21 04:30:56  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 */

#include "map.h"
#include "maperror.h"
#include "mapthread.h"

MS_CVSID("$Id$")

#ifdef USE_WCS_SVR

#include "gdal.h"
#include "cpl_string.h" /* GDAL string handling */

/*
** Structure to hold metadata taken from the image or image tile index
*/
typedef struct {
  const char *srs;
  rectObj extent, llextent;
  double geotransform[6];
  int xsize, ysize;
  double xresolution, yresolution;
  int bandcount; 
  int imagemode;
} coverageMetadataObj;

typedef struct {
  char *version;		/* 1.0.0 for now */
  char *request;		/* GetCapabilities|DescribeCoverage|GetCoverage */
  char *service;		/* MUST be WCS */
  char *section;		/* of capabilities document: /WCS_Capabilities/Service|/WCS_Capabilities/Capability|/WCS_Capabilities/ContentMetadata */
  char **coverages;		/* NULL terminated list of coverages (in the case of a GetCoverage there will only be 1) */
  char *crs;	        /* request coordinate reference system */
  char *response_crs;	/* response coordinate reference system */
  rectObj bbox;		    /* subset bounding box (3D), although we'll only use 2D */
  char *time;
  long width, height, depth;	/* image dimensions */
  double resx, resy, resz;      /* resolution */
  char *interpolation;          /* interpolationMethod */
  char *format;
  char *exceptions;		/* exception MIME type, (default application=vnd.ogc.se_xml) */
} wcsParamsObj;

/* functions defined later in this source file */
static int msWCSGetCoverageMetadata( layerObj *layer, coverageMetadataObj *cm );

#ifdef notdef /* currently unused */
/* value in a list (eg. is format valid) */
static int msWCSValidateParam(hashTableObj *metadata, char *name, const char *namespace, const char *value)
{
  return MS_SUCCESS; /* take their word for it at the moment */
}
#endif

/* RangeSets can be quite complex */
static int msWCSValidateRangeSetParam(hashTableObj *metadata, char *name, const char *namespace, const char *value)
{
  return MS_SUCCESS; /* take their word for it at the moment */
}

/************************************************************************/
/*                    msWCSConvertRangeSetToString()                    */
/************************************************************************/
static char *msWCSConvertRangeSetToString(const char *value) {
  char **tokens;
  int numtokens;
  double min, max, res;
  double val;
  char buf1[128], *buf2=NULL;

  if(strchr(value, '/')) { /* value is min/max/res */
    tokens = split(value, '/', &numtokens);
    if(tokens==NULL || numtokens != 3) {
      msFreeCharArray(tokens, numtokens);
      return NULL; /* not a set of equally spaced intervals */
    }
       
    min = atof(tokens[0]);
    max = atof(tokens[1]);
    res = atof(tokens[2]);
    msFreeCharArray(tokens, numtokens);
    
    for(val=min; val<=max; val+=res) {
      if(val == min)
        snprintf(buf1, 128, "%g", val);     
      else
        snprintf(buf1, 128, ",%g", val);
      buf2 = strcatalloc(buf2, buf1);
    }
   
    return buf2;
  } else
    return strdup(value);
}

/************************************************************************/
/*                           msWCSException()                           */
/************************************************************************/
static int msWCSException(mapObj *map, const char *version) 
{
  char *pszEncodedVal = NULL;
  /* msIO_printf("Content-type: application/vnd.ogc.se_xml%c%c",10,10); */
  msIO_printf("Content-type: text/xml%c%c",10,10);

  /* TODO: see WCS specific exception schema (Appendix 6), this is a copy of Dan's for WFS */

  msIO_printf("<ServiceExceptionReport\n");
  msIO_printf("xmlns=\"http://www.opengis.net/ogc\" "); 
  msIO_printf("xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "); 
  pszEncodedVal = msEncodeHTMLEntities(msOWSGetSchemasLocation(map));
  msIO_printf("xsi:schemaLocation=\"http://www.opengis.net/ogc %s/wms/1.1.1/OGC-exception.xsd\">\n", 
              pszEncodedVal);
  msFree(pszEncodedVal);
  msIO_printf("  <ServiceException>\n");
  msWriteErrorXML(stdout);
  msIO_printf("  </ServiceException>\n");
  msIO_printf("</ServiceExceptionReport>\n");

  return MS_FAILURE;
}

/************************************************************************/
/*                    msWCSPrintRequestCapability()                     */
/************************************************************************/

static void msWCSPrintRequestCapability(const char *version, const char *request_tag, const char *script_url)
{
  msIO_printf("    <%s>\n", request_tag);

  msIO_printf("      <DCPType>\n");
  msIO_printf("        <HTTP>\n");
  msIO_printf("          <Get><OnlineResource xlink:type=\"simple\" xlink:href=\"%s\" /></Get>\n", script_url);
  msIO_printf("        </HTTP>\n");
  msIO_printf("      </DCPType>\n");
  msIO_printf("      <DCPType>\n");
  msIO_printf("        <HTTP>\n");
  msIO_printf("          <Post><OnlineResource xlink:type=\"simple\" xlink:href=\"%s\" /></Post>\n", script_url);
  msIO_printf("        </HTTP>\n");
  msIO_printf("      </DCPType>\n");

  msIO_printf("    </%s>\n", request_tag);
}

/************************************************************************/
/*                         msWCSCreateParams()                          */
/************************************************************************/
static wcsParamsObj *msWCSCreateParams()
{
  wcsParamsObj *params;
 
  params = (wcsParamsObj *) calloc(1, sizeof(wcsParamsObj));
  if(params) { /* initialize a few things to default values */
    params->version = strdup("1.0.0");
  }
  
  return params;
}

/************************************************************************/
/*                          msWCSFreeParams()                           */
/************************************************************************/
static void msWCSFreeParams(wcsParamsObj *params)
{
  if(params) {
    /* TODO */
    if(params->version) free(params->version);
    if(params->request) free(params->request);
    if(params->service) free(params->service);
    if(params->section) free(params->section);
    if(params->crs) free(params->crs);
    if(params->response_crs) free(params->response_crs);      
    if(params->format) free(params->format);
    if(params->exceptions) free(params->exceptions);
    if(params->time) free(params->time);
    if(params->interpolation) free(params->interpolation);
  }
}

/************************************************************************/
/*                       msWCSIsLayerSupported()                        */
/************************************************************************/

static int msWCSIsLayerSupported(layerObj *layer)
{
    /* only raster layers, with 'DUMP TRUE' explicitly defined, are elligible to be served via WCS, WMS rasters are not ok */
    if(layer->dump && (layer->type == MS_LAYER_RASTER) && layer->connectiontype != MS_WMS) return MS_TRUE;

    return MS_FALSE;
}

/************************************************************************/
/*                      msWCSGetRequestParameter()                      */
/************************************************************************/

static const char *msWCSGetRequestParameter(cgiRequestObj *request, char *name) {
  int i;

  if(!request || !name) /* nothing to do */
	return NULL;

  if(request->NumParams > 0) {
    for(i=0; i<request->NumParams; i++) {
       if(strcasecmp(request->ParamNames[i], name) == 0)
	     return request->ParamValues[i];
    }
  }

  return NULL;
}

/************************************************************************/
/*                  msWCSSetDefaultBandsRangeSetInfo()                  */
/************************************************************************/

static void msWCSSetDefaultBandsRangeSetInfo( wcsParamsObj *params,
                                              coverageMetadataObj *cm,
                                              layerObj *lp ) 
{
    
    /* This function will provide default rangeset information for the "special" */
    /* "bands" rangeset if it appears in the axes list but has no specifics provided */
    /* in the metadata.   */

    const char *value;
    char *bandlist;
    int  i;

    /* Does this item exist in the axes list?  */

    value = msOWSLookupMetadata(&(lp->metadata), "COM", "rangeset_axes");
    if( value == NULL )
        return;

    value = strstr(value,"bands");
    if( value[5] != '\0' && value[5] != ' ' )
        return;

    /* Are there any w*s_bands_ metadata already? If so, skip out. */
    if( msOWSLookupMetadata(&(lp->metadata), "COM", "bands_description") != NULL
        || msOWSLookupMetadata(&(lp->metadata), "COM", "bands_name") != NULL
        || msOWSLookupMetadata(&(lp->metadata), "COM", "bands_label") != NULL
        || msOWSLookupMetadata(&(lp->metadata), "COM", "bands_values") != NULL
        || msOWSLookupMetadata(&(lp->metadata), "COM", "bands_values_semantic") != NULL
        || msOWSLookupMetadata(&(lp->metadata), "COM", "bands_values_type") != NULL
        || msOWSLookupMetadata(&(lp->metadata), "COM", "bands_rangeitem") != NULL
        || msOWSLookupMetadata(&(lp->metadata), "COM", "bands_semantic") != NULL
        || msOWSLookupMetadata(&(lp->metadata), "COM", "bands_refsys") != NULL
        || msOWSLookupMetadata(&(lp->metadata), "COM", "bands_refsyslabel") != NULL
        || msOWSLookupMetadata(&(lp->metadata), "COM", "bands_interval") != NULL )
        return;

    /* OK, we have decided to fill in the information. */

    msInsertHashTable( &(lp->metadata), "wcs_bands_name", "bands" );
    msInsertHashTable( &(lp->metadata), "wcs_bands_label", "Bands/Channels/Samples" );
    msInsertHashTable( &(lp->metadata), "wcs_bands_rangeitem", "_bands" ); /* ? */

    bandlist = (char *) malloc(cm->bandcount*30+30 );
    strcpy( bandlist, "1" );
    for( i = 1; i < cm->bandcount; i++ )
        sprintf( bandlist+strlen(bandlist),",%d", i+1 );
    
    msInsertHashTable( &(lp->metadata), "wcs_bands_values", bandlist );
    free( bandlist );
}    

/************************************************************************/
/*                         msWCSParseRequest()                          */
/************************************************************************/

static int msWCSParseRequest(cgiRequestObj *request, wcsParamsObj *params, mapObj *map)
{
  int i;

  if(!request || !params) /* nothing to do */
	return MS_SUCCESS;

  if(request->NumParams > 0) {
    for(i=0; i<request->NumParams; i++) {
    
       if(strcasecmp(request->ParamNames[i], "VERSION") == 0)
	     params->version = strdup(request->ParamValues[i]);
       else if(strcasecmp(request->ParamNames[i], "REQUEST") == 0)
	     params->request = strdup(request->ParamValues[i]);
       else if(strcasecmp(request->ParamNames[i], "INTERPOLATION") == 0)
	     params->interpolation = strdup(request->ParamValues[i]);
       else if(strcasecmp(request->ParamNames[i], "SERVICE") == 0)
	     params->service = strdup(request->ParamValues[i]);
	   else if(strcasecmp(request->ParamNames[i], "SECTION") == 0)
	     params->section = strdup(request->ParamValues[i]); /* TODO: validate value here */

       /* GetCoverage parameters. */
       else if(strcasecmp(request->ParamNames[i], "BBOX") == 0) {
         char **tokens;
         int n;
          
         tokens = split(request->ParamValues[i], ',', &n);
         if(tokens==NULL || n != 4) {
           msSetError(MS_WMSERR, "Wrong number of arguments for BBOX.", "msWCSParseRequest()");
           return msWCSException(map, params->version);
         }
         params->bbox.minx = atof(tokens[0]);
         params->bbox.miny = atof(tokens[1]);
         params->bbox.maxx = atof(tokens[2]);
         params->bbox.maxy = atof(tokens[3]);
           
         msFreeCharArray(tokens, n);
       } else if(strcasecmp(request->ParamNames[i], "RESX") == 0)
         params->resx = atof(request->ParamValues[i]);
       else if(strcasecmp(request->ParamNames[i], "RESY") == 0)
         params->resy = atof(request->ParamValues[i]);
       else if(strcasecmp(request->ParamNames[i], "WIDTH") == 0)
         params->width = atoi(request->ParamValues[i]);
       else if(strcasecmp(request->ParamNames[i], "HEIGHT") == 0)
         params->height = atoi(request->ParamValues[i]);
       else if(strcasecmp(request->ParamNames[i], "COVERAGE") == 0)
         params->coverages = CSLAddString(params->coverages, request->ParamValues[i]);
       else if(strcasecmp(request->ParamNames[i], "TIME") == 0)
           params->time = strdup(request->ParamValues[i]);
       else if(strcasecmp(request->ParamNames[i], "FORMAT") == 0)
           params->format = strdup(request->ParamValues[i]);
       else if(strcasecmp(request->ParamNames[i], "CRS") == 0)
           params->crs = strdup(request->ParamValues[i]);
       else if(strcasecmp(request->ParamNames[i], "RESPONSE_CRS") == 0)
           params->response_crs = strdup(request->ParamValues[i]);

	   
       /* and so on... */
    }
  }

  /* we are not dealing with an XML encoded request at this point */
  return MS_SUCCESS;
}

/************************************************************************/
/*           msWCSGetCapabilities_Service_ResponsibleParty()            */
/************************************************************************/

static void msWCSGetCapabilities_Service_ResponsibleParty(mapObj *map)
{
  int bEnableTelephone=MS_FALSE, bEnableAddress=MS_FALSE, bEnableOnlineResource=MS_FALSE;

  /* the WCS-specific way */
  if(msOWSLookupMetadata(&(map->web.metadata), "CO", "responsibleparty_individualname") || 
     msOWSLookupMetadata(&(map->web.metadata), "CO", "responsibleparty_organizationname")) {

    msIO_printf("<responsibleParty>\n");
    msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "responsibleparty_individualname", OWS_NOERR, "    <individualName>%s</individualName>\n", NULL);
    msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "responsibleparty_organizationname", OWS_NOERR, "    <organisationName>%s</organisationName>\n", NULL);
    msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "responsibleparty_positionname", OWS_NOERR, "    <positionName>%s</positionName>\n", NULL);

    if(msOWSLookupMetadata(&(map->web.metadata), "CO", "responsibleparty_phone_voice") ||
       msOWSLookupMetadata(&(map->web.metadata), "CO", "responsibleparty_phone_facsimile")) bEnableTelephone = MS_TRUE;

    if(msOWSLookupMetadata(&(map->web.metadata), "CO", "responsibleparty_address_deliverypoint") ||
       msOWSLookupMetadata(&(map->web.metadata), "CO", "responsibleparty_address_city") ||
       msOWSLookupMetadata(&(map->web.metadata), "CO", "responsibleparty_address_administrativearea") ||
       msOWSLookupMetadata(&(map->web.metadata), "CO", "responsibleparty_address_postalcode") ||
       msOWSLookupMetadata(&(map->web.metadata), "CO", "responsibleparty_address_country") ||
       msOWSLookupMetadata(&(map->web.metadata), "CO", "responsibleparty_address_electronicmailaddress")) bEnableAddress = MS_TRUE;
 
    if(msOWSLookupMetadata(&(map->web.metadata), "CO", "responsibleparty_onlineresource")) bEnableOnlineResource = MS_TRUE;

    if(bEnableTelephone || bEnableAddress || bEnableOnlineResource) {
      msIO_printf("  <contactInfo>\n");
      if(bEnableTelephone) {
	msIO_printf("    <phone>\n");
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "responsibleparty_phone_voice", OWS_NOERR, "    <voice>%s</voice>\n", NULL);
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "responsibleparty_phone_facsimile", OWS_NOERR, "    <facsimile>%s</facsimile>\n", NULL);
	msIO_printf("    </phone>\n");
      }
      if(bEnableAddress) {
	msIO_printf("    <address>\n");
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "responsibleparty_address_deliverypoint", OWS_NOERR, "    <deliveryPoint>%s</deliveryPoint>\n", NULL);
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "responsibleparty_address_city", OWS_NOERR, "    <city>%s</city>\n", NULL);
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "responsibleparty_address_administrativearea", OWS_NOERR, "    <administrativeArea>%s</administrativeArea>\n", NULL);
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "responsibleparty_address_postalcode", OWS_NOERR, "    <postalCode>%s</postalCode>\n", NULL);
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "responsibleparty_address_country", OWS_NOERR, "    <country>%s</country>\n", NULL);
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "responsibleparty_address_electronicmailaddress", OWS_NOERR, "    <electronicMailAddress>%s</electronicMailAddress>\n", NULL);
	msIO_printf("    </address>\n");
      }
      msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "responsibleparty_onlineresource", OWS_NOERR, "    <onlineResource xlink:type=\"simple\" xlink:href=\"%s\"/>\n", NULL);
      msIO_printf("  </contactInfo>\n");
    }

    msIO_printf("</responsibleParty>\n");

  } else if(msOWSLookupMetadata(&(map->web.metadata), "COM", "contactperson") ||
       msOWSLookupMetadata(&(map->web.metadata), "COM", "contactorganization")) { /* leverage WMS contact information */

    msIO_printf("<responsibleParty>\n");
    msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "COM", "contactperson", OWS_NOERR, "    <individualName>%s</individualName>\n", NULL);
    msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "COM", "contactorganization", OWS_NOERR, "    <organisationName>%s</organisationName>\n", NULL);
    msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "COM", "contactposition", OWS_NOERR, "    <positionName>%s</positionName>\n", NULL);

    if(msOWSLookupMetadata(&(map->web.metadata), "COM", "contactvoicetelephone") ||
       msOWSLookupMetadata(&(map->web.metadata), "COM", "contactfacsimiletelephone")) bEnableTelephone = MS_TRUE;

    if(msOWSLookupMetadata(&(map->web.metadata), "COM", "address") ||
       msOWSLookupMetadata(&(map->web.metadata), "COM", "city") ||
       msOWSLookupMetadata(&(map->web.metadata), "COM", "stateorprovince") ||
       msOWSLookupMetadata(&(map->web.metadata), "COM", "postcode") ||
       msOWSLookupMetadata(&(map->web.metadata), "COM", "country") ||
       msOWSLookupMetadata(&(map->web.metadata), "COM", "contactelectronicmailaddress")) bEnableAddress = MS_TRUE;

    if(msOWSLookupMetadata(&(map->web.metadata), "COM", "service_onlineresource")) bEnableOnlineResource = MS_TRUE;

    if(bEnableTelephone || bEnableAddress || bEnableOnlineResource) {
      msIO_printf("  <contactInfo>\n");
      if(bEnableTelephone) {
        msIO_printf("    <phone>\n");
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "COM", "contactvoicetelephone", OWS_NOERR, "    <voice>%s</voice>\n", NULL);
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "COM", "contactfacsimiletelephone", OWS_NOERR, "    <facsimile>%s</facsimile>\n", NULL);
        msIO_printf("    </phone>\n");
      }
      if(bEnableAddress) {
        msIO_printf("    <address>\n");
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "COM", "address", OWS_NOERR, "    <deliveryPoint>%s</deliveryPoint>\n", NULL);
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "COM", "city", OWS_NOERR, "    <city>%s</city>\n", NULL);
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "COM", "stateorprovince", OWS_NOERR, "    <administrativeArea>%s</administrativeArea>\n", NULL);
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "COM", "postcode", OWS_NOERR, "    <postalCode>%s</postalCode>\n", NULL);
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "COM", "country", OWS_NOERR, "    <country>%s</country>\n", NULL);
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "COM", "contactelectronicmailaddress", OWS_NOERR, "    <electronicMailAddress>%s</electronicMailAddress>\n", NULL);
        msIO_printf("    </address>\n");
      }
      msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "COM", "service_onlineresource", OWS_NOERR, "    <onlineResource xlink:type=\"simple\" xlink:href=\"%s\"/>\n", NULL);
      msIO_printf("  </contactInfo>\n");
    }
    msIO_printf("</responsibleParty>\n");
  }

  return;
}

/************************************************************************/
/*                    msWCSGetCapabilities_Service()                    */
/************************************************************************/

static int msWCSGetCapabilities_Service(mapObj *map, wcsParamsObj *params)
{
  /* start the Service section, only need the full start tag if this is the only section requested */
  if(!params->section) 
    msIO_printf("<Service>\n");
  else
    msIO_printf("<Service\n"
           "   version=\"%s\" \n"
           "   updateSequence=\"0\" \n"
           "   xmlns=\"http://www.opengis.net/wcs\" \n" 
           "   xmlns:xlink=\"http://www.w3.org/1999/xlink\" \n" 
           "   xmlns:gml=\"http://www.opengis.net/gml\" \n" 
           "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n" 
           "   xsi:schemaLocation=\"http://www.opengis.net/wcs %s/wcs/%s/wcsCapabilities.xsd\">\n", params->version, msOWSGetSchemasLocation(map), params->version); 

  /* optional metadataLink */
  msOWSPrintURLType(stdout, &(map->web.metadata), "COM", "metadatalink", 
                    OWS_NOERR, 
                    "  <metadataLink%s%s%s%s xlink:type=\"simple\"%s/>", 
                    NULL, " metadataType=\"%s\"", NULL, NULL, NULL,  
                    " xlink:href=\"%s\"", MS_FALSE, MS_FALSE, MS_FALSE, 
                    MS_FALSE, MS_TRUE, "other", NULL, NULL, NULL, NULL, NULL);

  msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "COM", "description", OWS_NOERR, "  <description>%s</description>\n", NULL);
  msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "COM", "name", OWS_NOERR, "  <name>%s</name>\n", "MapServer WCS");
  msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "COM", "label", OWS_WARN, "  <label>%s</label>\n", NULL);

  /* we are not supporting the optional keyword type, at least not yet */
  msOWSPrintEncodeMetadataList(stdout, &(map->web.metadata), "COM", "keywordlist", "  <keywords>\n", "  </keywords>\n", "    <keyword>%s</keyword>\n", NULL);

  msWCSGetCapabilities_Service_ResponsibleParty(map);

  msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "COM", "fees", OWS_NOERR, "  <fees>%s</fees>\n", "NONE");
  msOWSPrintEncodeMetadataList(stdout, &(map->web.metadata), "COM", "accessconstraints", "  <accessConstraints>\n", "  </accessConstraints>\n", "    %s\n", "NONE");

  /* done */
  msIO_printf("</Service>\n");

  return MS_SUCCESS;
}

/************************************************************************/
/*                  msWCSGetCapabilities_Capability()                   */
/************************************************************************/

static int msWCSGetCapabilities_Capability(mapObj *map, wcsParamsObj *params, cgiRequestObj *req)
{
  char *script_url=NULL, *script_url_encoded;

   /* we need this server's onlineresource for the request section */
  if((script_url=msOWSGetOnlineResource(map, "CO", "onlineresource", req)) == NULL || (script_url_encoded = msEncodeHTMLEntities(script_url)) == NULL) {
    return msWCSException(map, params->version);
  }

  /* start the Capabilty section, only need the full start tag if this is the only section requested */
  if(!params->section) 
    msIO_printf("<Capability>\n");
  else
    msIO_printf("<Capability\n"
           "   version=\"%s\" \n"
           "   updateSequence=\"0\" \n"
           "   xmlns=\"http://www.opengis.net/wcs\" \n" 
           "   xmlns:xlink=\"http://www.w3.org/1999/xlink\" \n" 
           "   xmlns:gml=\"http://www.opengis.net/gml\" \n" 
           "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n" 
           "   xsi:schemaLocation=\"http://www.opengis.net/wcs %s/wcs/%s/wcsCapabilities.xsd\">\n", params->version, msOWSGetSchemasLocation(map), params->version); 

  /* describe the types of requests the server can handle */
  msIO_printf("  <Request>\n");

  msWCSPrintRequestCapability(params->version, "GetCapabilities", script_url_encoded);
  msWCSPrintRequestCapability(params->version, "DescribeCoverage", script_url_encoded);
  msWCSPrintRequestCapability(params->version, "GetCoverage", script_url_encoded);
 
  msIO_printf("  </Request>\n");

  /* describe the exception formats the server can produce */
  msIO_printf("  <Exception>\n");
  msIO_printf("    <Format>application/vnd.ogc.se_xml</Format>\n");
  msIO_printf("  </Exception>\n");

  /* describe any vendor specific capabilities */
   msIO_printf("  <VendorSpecificCapabilities />\n"); /* none yet */

  /* done */
  msIO_printf("</Capability>\n");

  return MS_SUCCESS;
}

/************************************************************************/
/*             msWCSGetCapabilities_CoverageOfferingBrief()             */
/************************************************************************/

static int msWCSGetCapabilities_CoverageOfferingBrief(layerObj *layer, wcsParamsObj *params) 
{
  coverageMetadataObj cm;
  int status;

  if(!msWCSIsLayerSupported(layer)) return MS_SUCCESS; /* not an error, this layer cannot be served via WCS */

  status = msWCSGetCoverageMetadata(layer, &cm);
  if(status != MS_SUCCESS) return MS_FAILURE;
 
  /* start the CoverageOfferingBrief section */
  msIO_printf("  <CoverageOfferingBrief>\n"); /* is this tag right? (I hate schemas without ANY examples) */

  /* optional metadataLink */
  msOWSPrintURLType(stdout, &(layer->metadata), "COM", "metadatalink", 
                    OWS_NOERR, 
                    "  <metadataLink%s%s%s%s xlink:type=\"simple\"%s/>", 
                    NULL, " metadataType=\"%s\"", NULL, NULL, NULL,  
                    " xlink:href=\"%s\"", MS_FALSE, MS_FALSE, MS_FALSE, 
                    MS_FALSE, MS_TRUE, "other", NULL, NULL, NULL, NULL, NULL);

  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "COM", "description", OWS_NOERR, "  <description>%s</description>\n", NULL);
  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "COM", "name", OWS_NOERR, "  <name>%s</name>\n", layer->name);

  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "COM", "label", OWS_WARN, "  <label>%s</label>\n", NULL);

  /* TODO: add elevation and temporal ranges to lonLatEnvelope (optional) */
  msIO_printf("    <lonLatEnvelope srsName=\"WGS84(DD)\">\n");
  msIO_printf("      <gml:pos>%.15g %.15g</gml:pos>\n", cm.llextent.minx, cm.llextent.miny); /* TODO: don't know if this is right */
  msIO_printf("      <gml:pos>%.15g %.15g</gml:pos>\n", cm.llextent.maxx, cm.llextent.maxy);
  msIO_printf("    </lonLatEnvelope>\n");

  /* we are not supporting the optional keyword type, at least not yet */
  msOWSPrintEncodeMetadataList(stdout, &(layer->metadata), "COM", "keywordlist", "  <keywords>\n", "  </keywords>\n", "    <keyword>%s</keyword>\n", NULL);

  /* done */
  msIO_printf("  </CoverageOfferingBrief>\n");

  return MS_SUCCESS;
}

/************************************************************************/
/*                msWCSGetCapabilities_ContentMetadata()                */
/************************************************************************/

static int msWCSGetCapabilities_ContentMetadata(mapObj *map, wcsParamsObj *params)
{
  int i;

  /* start the ContentMetadata section, only need the full start tag if this is the only section requested */
  /* TODO: add Xlink attributes for other sources of this information  */
  if(!params->section)
    msIO_printf("<ContentMetadata>\n");
  else
    msIO_printf("<ContentMetadata\n"
           "   version=\"%s\" \n"
           "   updateSequence=\"0\" \n"
           "   xmlns=\"http://www.opengis.net/wcs\" \n" 
           "   xmlns:xlink=\"http://www.w3.org/1999/xlink\" \n" 
           "   xmlns:gml=\"http://www.opengis.net/gml\" \n" 
           "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n" 
           "   xsi:schemaLocation=\"http://www.opengis.net/wcs %s/wcs/%s/wcsCapabilities.xsd\">\n", params->version, msOWSGetSchemasLocation(map), params->version); 

  for(i=0; i<map->numlayers; i++)
    msWCSGetCapabilities_CoverageOfferingBrief(&(map->layers[i]), params);

  /* done */
  msIO_printf("</ContentMetadata>\n");

  return MS_SUCCESS;
}

/************************************************************************/
/*                        msWCSGetCapabilities()                        */
/************************************************************************/

static int msWCSGetCapabilities(mapObj *map, wcsParamsObj *params, cgiRequestObj *req)
{
  /* msIO_printf("Content-type: application/vnd.ogc.se_xml%c%c",10,10); */
  msIO_printf("Content-type: text/xml%c%c",10,10);

  /* print common capability elements  */
  /* TODO: DocType? */
  
  msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), NULL, "wcs_encoding", OWS_NOERR, "<?xml version='1.0' encoding=\"%s\" standalone=\"no\" ?>\n", "ISO-8859-1");

  if(!params->section) msIO_printf("<WCS_Capabilities\n"
                              "   version=\"%s\" \n"
                              "   updateSequence=\"0\" \n"
                              "   xmlns=\"http://www.opengis.net/wcs\" \n" 
                              "   xmlns:xlink=\"http://www.w3.org/1999/xlink\" \n" 
                              "   xmlns:gml=\"http://www.opengis.net/gml\" \n" 
                              "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n" 
                              "   xsi:schemaLocation=\"http://www.opengis.net/wcs %s/wcs/%s/wcsCapabilities.xsd\">\n", params->version, msOWSGetSchemasLocation(map), params->version); 
         
  /* print the various capability sections */
  if(!params->section || strcasecmp(params->section, "/WCS_Capabilities/Service") == 0)
    msWCSGetCapabilities_Service(map, params);

  if(!params->section || strcasecmp(params->section, "/WCS_Capabilities/Capability")  == 0)
    msWCSGetCapabilities_Capability(map, params, req);

  if(!params->section || strcasecmp(params->section, "/WCS_Capabilities/ContentMetadata")  == 0)
    msWCSGetCapabilities_ContentMetadata(map, params);

  /* done */
  if(!params->section) msIO_printf("</WCS_Capabilities>\n");

  /* clean up */
  /* msWCSFreeParams(params); */

  return MS_SUCCESS;
}

/************************************************************************/
/*               msWCSDescribeCoverage_AxisDescription()                */
/************************************************************************/

static int msWCSDescribeCoverage_AxisDescription(layerObj *layer, char *name)
{
  const char *value;
  char tag[100]; /* should be plenty of space */
  
  msIO_printf("        <axisDescription>\n");
  msIO_printf("          <AxisDescription");
  snprintf(tag, 100, "%s_semantic",  name); /* optional attributes follow (should escape?) */
  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "COM", tag, OWS_NOERR, " semantic=\"%s\"", NULL);
  snprintf(tag, 100, "%s_refsys", name);
  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "COM", tag, OWS_NOERR, " refSys=\"%s\"", NULL);
  snprintf(tag, 100, "%s_refsyslabel", name);
  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "COM", tag, OWS_NOERR, " refSysLabel=\"%s\"", NULL);
  msIO_printf(">\n");
  
  /* TODO: add metadataLink (optional) */
  
  snprintf(tag, 100, "%s_description", name);
  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "COM", tag, OWS_NOERR, "            <description>%s</description>\n", NULL);
  /* snprintf(tag, 100, "%s_name", name); */
  /* msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "COM", tag, OWS_WARN, "            <name>%s</name>\n", NULL); */
  msIO_printf("            <name>%s</name>\n", name);
 
  snprintf(tag, 100, "%s_label", name);
  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "COM", tag, OWS_WARN, "            <label>%s</label>\n", NULL);
  
  /* Values */
  msIO_printf("            <values");
  snprintf(tag, 100, "%s_values_semantic", name); /* optional attributes follow (should escape?) */
  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "COM", tag, OWS_NOERR, " semantic=\"%s\"", NULL);
  snprintf(tag, 100, "%s_values_type", name);
  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "COM", tag, OWS_NOERR, " type=\"%s\"", NULL);
  msIO_printf(">\n");
  
  /* single values, we do not support optional type and semantic attributes */
  snprintf(tag, 100, "%s_values", name);
  if(msOWSLookupMetadata(&(layer->metadata), "COM", tag))
    msOWSPrintEncodeMetadataList(stdout, &(layer->metadata), "COM", tag, NULL, NULL, "              <singleValue>%s</singleValue>\n", NULL);
  
  /* intervals, only one per axis for now, we do not support optional type, atomic and semantic attributes */
  snprintf(tag, 100, "%s_interval", name);
  if((value = msOWSLookupMetadata(&(layer->metadata), "COM", tag)) != NULL) {
    char **tokens;
    int numtokens;

     tokens = split(value, '/', &numtokens);
     if(tokens && numtokens > 0) {
       msIO_printf("            <interval>\n");
       if(numtokens >= 1) msIO_printf("            <min>%s</min>\n", tokens[0]); /* TODO: handle closure */
       if(numtokens >= 2) msIO_printf("            <max>%s</max>\n", tokens[1]);
       if(numtokens >= 3) msIO_printf("            <res>%s</res>\n", tokens[2]);
       msIO_printf("            </interval>\n"); 
     }
  }
  
  /* TODO: add default (optional) */
  
  msIO_printf("            </values>\n");
  
  /* TODO: add NullValues (optional) */
  
  msIO_printf("          </AxisDescription>\n");
  msIO_printf("        </axisDescription>\n");
  
  return MS_SUCCESS;
}

/************************************************************************/
/*               msWCSDescribeCoverage_CoverageOffering()               */
/************************************************************************/

static int msWCSDescribeCoverage_CoverageOffering(layerObj *layer, wcsParamsObj *params) 
{
  char **tokens;
  int numtokens;
  const char *value; 
  coverageMetadataObj cm;
  int i, status;

  if(!msWCSIsLayerSupported(layer)) return MS_SUCCESS; /* not an error, this layer cannot be served via WCS */

  status = msWCSGetCoverageMetadata(layer, &cm);
  if(status != MS_SUCCESS) return MS_FAILURE;

  /* fill in bands rangeset info, if required.  */
  msWCSSetDefaultBandsRangeSetInfo( params, &cm, layer );
  
  /* start the Coverage section */
  msIO_printf("  <CoverageOffering>\n");

  /* optional metadataLink */
  msOWSPrintURLType(stdout, &(layer->metadata), "COM", "metadatalink",
                    OWS_NOERR,
                    "  <metadataLink%s%s%s%s xlink:type=\"simple\"%s/>",
                    NULL, " metadataType=\"%s\"", NULL, NULL, NULL,
                    " xlink:href=\"%s\"", MS_FALSE, MS_FALSE, MS_FALSE,
                    MS_FALSE, MS_TRUE, "other", NULL, NULL, NULL, NULL, NULL);
  
  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "COM", "description", OWS_NOERR, "  <description>%s</description>\n", NULL);
  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "COM", "name", OWS_NOERR, "  <name>%s</name>\n", layer->name);

  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "COM", "label", OWS_WARN, "  <label>%s</label>\n", NULL);

  /* TODO: add elevation and temporal ranges to lonLatEnvelope (optional) */
  msIO_printf("    <lonLatEnvelope srsName=\"WGS84(DD)\">\n");
  msIO_printf("      <gml:pos>%.15g %.15g</gml:pos>\n", cm.llextent.minx, cm.llextent.miny);
  msIO_printf("      <gml:pos>%.15g %.15g</gml:pos>\n", cm.llextent.maxx, cm.llextent.maxy);
  msIO_printf("    </lonLatEnvelope>\n");

  /* we are not supporting the optional keyword type, at least not yet */
  msOWSPrintEncodeMetadataList(stdout, &(layer->metadata), "COM", "keywordlist", "  <keywords>\n", "  </keywords>\n", "    <keyword>%s</keyword>\n", NULL);

  /* DomainSet: starting simple, just a spatial domain (gml:envelope) and optionally a temporal domain */
  msIO_printf("    <domainSet>\n");

  /* SpatialDomain */
  msIO_printf("      <spatialDomain>\n");
  
  /* envelope in lat/lon */
  msIO_printf("        <gml:Envelope srsName=\"WGS84(DD)\">\n");
  msIO_printf("          <gml:pos>%.15g %.15g</gml:pos>\n", cm.llextent.minx, cm.llextent.miny);
  msIO_printf("          <gml:pos>%.15g %.15g</gml:pos>\n", cm.llextent.maxx, cm.llextent.maxy);
  msIO_printf("        </gml:Envelope>\n");
  
  /* envelope in the native srs */
  if((value = msOWSGetEPSGProj(&(layer->projection), &(layer->metadata), "COM", MS_TRUE)) != NULL)    
    msIO_printf("        <gml:Envelope srsName=\"%s\">\n", value);
  else if((value = msOWSGetEPSGProj(&(layer->map->projection), &(layer->map->web.metadata), "COM", MS_TRUE)) != NULL)
    msIO_printf("        <gml:Envelope srsName=\"%s\">\n", value);
  else 
    msIO_printf("        <!-- NativeCRSs ERROR: missing required information, no SRSs defined -->\n");
  msIO_printf("          <gml:pos>%.15g %.15g</gml:pos>\n", cm.extent.minx, cm.extent.miny);
  msIO_printf("          <gml:pos>%.15g %.15g</gml:pos>\n", cm.extent.maxx, cm.extent.maxy);
  msIO_printf("        </gml:Envelope>\n");

  /* gml:rectifiedGrid */
  msIO_printf("        <gml:RectifiedGrid dimension=\"2\">\n");
  msIO_printf("          <gml:limits>\n");
  msIO_printf("            <gml:GridEnvelope>\n");
  msIO_printf("              <gml:low>0 0</gml:low>\n");
  msIO_printf("              <gml:high>%d %d</gml:high>\n", cm.xsize-1, cm.ysize-1);
  msIO_printf("            </gml:GridEnvelope>\n");
  msIO_printf("          </gml:limits>\n");
  msIO_printf("          <gml:axisName>x</gml:axisName>\n");
  msIO_printf("          <gml:axisName>y</gml:axisName>\n");
  msIO_printf("          <gml:origin>\n");
  msIO_printf("            <gml:pos>%.15g %.15g</gml:pos>\n", cm.geotransform[0], cm.geotransform[3]);
  msIO_printf("          </gml:origin>\n");
  msIO_printf("          <gml:offsetVector>%.15g %.15g</gml:offsetVector>\n", cm.geotransform[1], cm.geotransform[2]); /* offset vector in X direction */
  msIO_printf("          <gml:offsetVector>%.15g %.15g</gml:offsetVector>\n", cm.geotransform[4], cm.geotransform[5]); /* offset vector in Y direction */
  msIO_printf("        </gml:RectifiedGrid>\n");

  msIO_printf("      </spatialDomain>\n");

  /* TemporalDomain */

  /* TODO: figure out when a temporal domain is valid, for example only tiled rasters support time as a domain, plus we need a timeitem */
  if(msOWSLookupMetadata(&(layer->metadata), "COM", "timeposition") || msOWSLookupMetadata(&(layer->metadata), "COM", "timeperiod")) {
    msIO_printf("      <temporalDomain>\n");

    /* TimePosition (should support a value AUTO, then we could mine positions from the timeitem) */
    msOWSPrintEncodeMetadataList(stdout, &(layer->metadata), "COM", "timeposition", NULL, NULL, "        <gml:timePosition>%s</gml:timePosition>\n", NULL);    
    /* TODO:  add TimePeriod (only one per layer)  */

    msIO_printf("      </temporalDomain>\n");
  }
  
  msIO_printf("    </domainSet>\n");
  
  /* rangeSet */
  msIO_printf("    <rangeSet>\n");
  msIO_printf("      <RangeSet>\n"); /* TODO: there are some optional attributes */

  /* TODO: add metadataLink (optional) */
  
  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "COM", "rangeset_description", OWS_NOERR, "        <description>%s</description>\n", NULL);
  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "COM", "rangeset_name", OWS_WARN, "        <name>%s</name>\n", NULL);

  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "COM", "rangeset_label", OWS_WARN, "        <label>%s</label>\n", NULL);
  
  /* compound range sets */
  if((value = msOWSLookupMetadata(&(layer->metadata), "COM", "rangeset_axes")) != NULL) {
    tokens = split(value, ',', &numtokens);
    if(tokens && numtokens > 0) {
      for(i=0; i<numtokens; i++)
       msWCSDescribeCoverage_AxisDescription(layer, tokens[i]);
      msFreeCharArray(tokens, numtokens);
    }
  }
  
  msIO_printf("      </RangeSet>\n");
  msIO_printf("    </rangeSet>\n");

  /* supportedCRSs */
  msIO_printf("    <supportedCRSs>\n");
  
  /* requestResposeCRSs: check the layer metadata/projection, and then the map metadata/projection if necessary (should never get to the error message) */
  if((value = msOWSGetEPSGProj(&(layer->projection), &(layer->metadata), "COM", MS_FALSE)) != NULL) {
    tokens = split(value, ' ', &numtokens);
    if(tokens && numtokens > 0) {
      for(i=0; i<numtokens; i++)
        msIO_printf("      <requestResponseCRSs>%s</requestResponseCRSs>\n", tokens[i]);
      msFreeCharArray(tokens, numtokens);
    }
  } else if((value = msOWSGetEPSGProj(&(layer->map->projection), &(layer->map->web.metadata), "COM", MS_FALSE)) != NULL) {
    tokens = split(value, ' ', &numtokens);
    if(tokens && numtokens > 0) {
      for(i=0; i<numtokens; i++)
        msIO_printf("      <requestResponseCRSs>%s</requestResponseCRSs>\n", tokens[i]);
      msFreeCharArray(tokens, numtokens);
    }
  } else 
    msIO_printf("      <!-- requestResponseCRSs ERROR: missing required information, no SRSs defined -->\n");
  
  /* nativeCRSs (only one in our case) */
  if((value = msOWSGetEPSGProj(&(layer->projection), &(layer->metadata), "COM", MS_TRUE)) != NULL)    
    msIO_printf("      <nativeCRSs>%s</nativeCRSs>\n", value);
  else if((value = msOWSGetEPSGProj(&(layer->map->projection), &(layer->map->web.metadata), "COM", MS_TRUE)) != NULL)
    msIO_printf("      <nativeCRSs>%s</nativeCRSs>\n", value);
  else 
    msIO_printf("      <!-- nativeCRSs ERROR: missing required information, no SRSs defined -->\n");
  
  msIO_printf("    </supportedCRSs>\n");
  
  
  /* supportedFormats */
  msIO_printf("    <supportedFormats");
  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "COM", "nativeformat", OWS_NOERR, " nativeFormat=\"%s\"", NULL);
  msIO_printf(">\n");
  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "COM", "formats", OWS_NOERR, "      <formats>%s</formats>\n", NULL);
  msIO_printf("    </supportedFormats>\n");
  
  msIO_printf("    <supportedInterpolations default=\"nearest neighbour\">\n");
  msIO_printf("      <interpolationMethod>nearest neighbor</interpolationMethod>\n" );
  msIO_printf("      <interpolationMethod>bilinear</interpolationMethod>\n" );
/*  msIO_printf("      <interpolationMethod>bicubic</interpolationMethod>\n" ); */
  msIO_printf("    </supportedInterpolations>\n");
  

  /* done */
  msIO_printf("  </CoverageOffering>\n");

  return MS_SUCCESS;
}

/************************************************************************/
/*                       msWCSDescribeCoverage()                        */
/************************************************************************/

static int msWCSDescribeCoverage(mapObj *map, wcsParamsObj *params)
{
  int i,j;
  
  /* printf("Content-type: application/vnd.ogc.se_xml%c%c",10,10); */
  msIO_printf("Content-type: text/xml%c%c",10,10);

  /* print common capability elements  */
  msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), NULL, "wcs_encoding", OWS_NOERR, "<?xml version='1.0' encoding=\"%s\" ?>\n", "ISO-8859-1");
  
  /* start the DescribeCoverage section */
  msIO_printf("<CoverageDescription\n"
         "   version=\"%s\" \n"
         "   updateSequence=\"0\" \n"
         "   xmlns=\"http://www.opengis.net/wcs\" \n" 
         "   xmlns:xlink=\"http://www.w3.org/1999/xlink\" \n" 
         "   xmlns:gml=\"http://www.opengis.net/gml\" \n" 
         "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n" 
         "   xsi:schemaLocation=\"http://www.opengis.net/wcs %s/wcs/%s/describeCoverage.xsd\">\n", params->version, msOWSGetSchemasLocation(map), params->version); 
  
  if(params->coverages) { /* use the list */
    for( j = 0; params->coverages[j]; j++ ) {
      i = msGetLayerIndex(map, params->coverages[j]);
      if(i == -1) continue; /* skip this layer, could/should generate an error */
      msWCSDescribeCoverage_CoverageOffering(&(map->layers[i]), params);
    }
  } else { /* return all layers */
    for(i=0; i<map->numlayers; i++)
      msWCSDescribeCoverage_CoverageOffering(&(map->layers[i]), params);
  }
  
  /* done */
  msIO_printf("</CoverageDescription>\n");
  
  return MS_SUCCESS;
}

/************************************************************************/
/*                          msWCSGetCoverage()                          */
/************************************************************************/

static int msWCSGetCoverage(mapObj *map, cgiRequestObj *request, 
                            wcsParamsObj *params)
{
  imageObj   *image;
  layerObj   *lp;
  int         status, i;
  const char *value;
  outputFormatObj *format;
  char *bandlist=NULL;
  char numbands[8]; /* should be large enough to hold the number of bands in the bandlist */
  coverageMetadataObj cm;

  /* make sure all required parameters are available (at least the easy ones) */
  if(!params->crs) {
    msSetError( MS_WCSERR, "Required parameter CRS was not supplied.", "msWCSGetCoverage()");
    return msWCSException(map, params->version);
  }

  if( params->coverages == NULL || params->coverages[0] == NULL ) {
    msSetError( MS_WCSERR, 
                "Required parameter COVERAGE was not supplied.", 
                "msWCSGetCoverage()");
    return msWCSException(map, params->version);
  }
  
  /* find the layer we are working with.  */
  lp = NULL;
  for(i=0; i<map->numlayers; i++) {
    if( EQUAL(map->layers[i].name, params->coverages[0]) ) {
      lp = map->layers + i;
      break;
    }
  }

  if(lp == NULL) {
    msSetError( MS_WCSERR, "COVERAGE=%s not found, not in supported layer list.", "msWCSGetCoverage()", params->coverages[0] );
    return msWCSException(map, params->version);
  }

  /* make sure the layer is on */
  lp->status = MS_ON;

  /* we need the coverage metadata, since things like numbands may not be available otherwise */
  status = msWCSGetCoverageMetadata(lp, &cm);
  if(status != MS_SUCCESS) return MS_FAILURE;

  /* fill in bands rangeset info, if required.  */
  msWCSSetDefaultBandsRangeSetInfo(params, &cm, lp);

  /* handle the response CRS, that is set the map object projection */
  if(params->response_crs || params->crs ) {
    const char *crs_to_use = params->response_crs;
    if( crs_to_use == NULL )
      crs_to_use = params->crs;

    if (strncasecmp(crs_to_use, "EPSG:", 5) == 0) {
      /* RESPONSE_CRS=EPSG:xxxx */
      char buffer[32];
      int iUnits;

      sprintf(buffer, "init=epsg:%.20s", crs_to_use+5);

      if (msLoadProjectionString(&(map->projection), buffer) != 0)
        return msWCSException( map, params->version );
        
      iUnits = GetMapserverUnitUsingProj(&(map->projection));
      if (iUnits != -1)
        map->units = iUnits;
    } else {  /* should we support WMS style AUTO: projections? (not for now) */
      msSetError(MS_WMSERR, "Unsupported SRS namespace (only EPSG currently supported).", "msWCSGetCoverage()");
      return msWCSException(map, params->version);
    }
  }

  /* did we get a TIME value (support only a single value for now) */
  if(params->time) {
    int tli;
    layerObj *tlp=NULL;
    
    /* need to handle NOW case */

    /* check format of TIME parameter */
    if(strchr(params->time, ',')) {
      msSetError( MS_WCSERR, "Temporal lists are not supported, only individual values.", "msWCSGetCoverage()" );
      return msWCSException(map, params->version);
    }
    if(strchr(params->time, '/')) {
      msSetError( MS_WCSERR, "Temporal ranges are not supported, only individual values.", "msWCSGetCoverage()" );
      return msWCSException(map, params->version);
    }
      
    /* TODO: will need to expand this check if a time period is supported */
    value = msOWSLookupMetadata(&(lp->metadata), "COM", "timeposition");
    if(!value) {
      msSetError( MS_WCSERR, "The coverage does not support temporal subsetting.", "msWCSGetCoverage()" );
      return msWCSException(map, params->version);
    }
    if(!strstr(value, params->time)) { /* this is likely too simple a test */
      msSetError( MS_WCSERR, "The coverage does not have a time position of %s.", "msWCSGetCoverage()", params->time );
      return msWCSException(map, params->version);
    }
      
    /* make sure layer is tiled appropriately */
    if(!lp->tileindex) {
      msSetError( MS_WCSERR, "Underlying layer is not tiled, unable to do temporal subsetting.", "msWCSGetCoverage()" );
      return msWCSException(map, params->version);
    }
    tli = msGetLayerIndex(map, lp->tileindex);
    if(tli == -1) {
      msSetError( MS_WCSERR, "Underlying layer does not use appropriate tiling mechanism.", "msWCSGetCoverage()" );
      return msWCSException(map, params->version);
    }

    tlp = &(map->layers[tli]);

    /* make sure there is enough information to filter */
    value = msOWSLookupMetadata(&(lp->metadata), "COM", "timeitem");
    if(!tlp->filteritem && !value) {
      msSetError( MS_WCSERR, "Not enough information available to filter.", "msWCSGetCoverage()" );
      return msWCSException(map, params->version);
    }
      
    /* override filteritem if specified in metadata */
    if(value) {
      if(tlp->filteritem) free(tlp->filteritem);
      tlp->filteritem = strdup(value);
    }
      
    /* finally set the filter */
    freeExpression(&tlp->filter);
    msLoadExpressionString(&tlp->filter, params->time);
  }
           
  /* Are there any non-spatio/temporal ranges to do subsetting on (e.g. bands) */
  value = msOWSLookupMetadata(&(lp->metadata), "COM", "rangeset_axes"); /* this will get all the compound range sets */
  if(value) { 
    char **tokens;
    int numtokens;
    char tag[100];
    const char *rangeitem;

    tokens = split(value, ',', &numtokens);
    for(i=0; i<numtokens; i++) {
      if((value = msWCSGetRequestParameter(request, tokens[i])) == NULL) continue; /* next rangeset parameter */
       
      if(msWCSValidateRangeSetParam(&(lp->metadata), tokens[i], "COM", value) != MS_SUCCESS) {
        msSetError( MS_WCSERR, "Error specifying \"%s\" paramter value(s).", "msWCSGetCoverage()", tokens[i]);
        return msWCSException(map, params->version);
      }
       
      /* xxxxx_rangeitem tells us how to subset */
      snprintf(tag, 100, "%s_rangeitem", tokens[i]);
      if((rangeitem = msOWSLookupMetadata(&(lp->metadata), "COM", tag)) == NULL) {
        msSetError( MS_WCSERR, "Missing required metadata element \"%s\", unable to process %s=%s.", "msWCSGetCoverage()", tag, tokens[i], value);
        return msWCSException(map, params->version);
      }
         
      if(strcasecmp(rangeitem, "_bands") == 0) { /* special case, subset bands */
        bandlist = msWCSConvertRangeSetToString(value);
           
        if(!bandlist) {
          msSetError( MS_WCSERR, "Error specifying \"%s\" paramter value(s).", "msWCSGetCoverage()", tokens[i]);
          return msWCSException(map, params->version);
        }          
      } else if(strcasecmp(rangeitem, "_pixels") == 0) { /* special case, subset pixels */
        msSetError( MS_WCSERR, "Arbitrary range sets based on pixel values are not yet supported.", "msWCSGetCoverage()" );
        return msWCSException(map, params->version);
      } else {
        msSetError( MS_WCSERR, "Arbitrary range sets based on tile (i.e. image) attributes are not yet supported.", "msWCSGetCoverage()" );
        return msWCSException(map, params->version);
      }
    }
       
    /* clean-up */
    msFreeCharArray(tokens, numtokens);
  }
    
  /* did we get BBOX values? if not use the exent stored in the coverageMetadataObj */
  if( fabs((params->bbox.maxx - params->bbox.minx)) < 0.000000000001  || fabs(params->bbox.maxy - params->bbox.miny) < 0.000000000001 ) {
    params->bbox = cm.extent;
    /* msSetError( MS_WCSERR, "Required parameter BBOX missing or specifies an empty region.", "msWCSGetCoverage()" ); */
    /* return msWCSException(params->version); */
  }
    
  /* if necessary, project the BBOX */
    
  /* compute width/height from BBOX and cellsize.  */
  if( (params->resx == 0.0 || params->resy == 0.0) && params->width != 0 && params->height != 0 ) {
    params->resx = (params->bbox.maxx - params->bbox.minx) / params->width;
    params->resy = (params->bbox.maxy - params->bbox.miny) / params->height;
  }
    
  /* compute cellsize/res from bbox and raster size. */
  if( (params->width == 0 || params->height == 0) && params->resx != 0 && params->resy != 0 ) {
    params->width = (int) ((params->bbox.maxx - params->bbox.minx) / params->resx + 0.5);
    params->height = (int) ((params->bbox.maxy - params->bbox.miny) / params->resy + 0.5);
  }

  /* are we still underspecified?  */
  if( params->width == 0 || params->height == 0 || params->resx == 0.0 || params->resy == 0.0 ) {
    msSetError( MS_WCSERR, "A non-zero RESX/RESY or WIDTH/HEIGHT is required but neither was provided.", "msWCSGetCoverage()" );
    return msWCSException(map, params->version);
  }

  map->cellsize = params->resx;

  /* Do we need to force special handling?  */
  if( fabs(params->resx/params->resy - 1.0) > 0.001 ) {
      map->gt.need_geotransform = MS_TRUE;
      if( map->debug )
          msDebug( "RESX and RESY don't match.  Using geotransform/resample.");
  }

  /* Do we have a specified interpolation method */
  if( params->interpolation != NULL )
  {
      if( strncasecmp(params->interpolation,"NEAREST",7) == 0 )
          msLayerSetProcessingKey(lp, "RESAMPLE", "NEAREST");
      else if( strcasecmp(params->interpolation,"BILINEAR") == 0 )
          msLayerSetProcessingKey(lp, "RESAMPLE", "BILINEAR");
      else if( strcasecmp(params->interpolation,"AVERAGE") == 0 )
          msLayerSetProcessingKey(lp, "RESAMPLE", "AVERAGE");
      else
      {
          msSetError( MS_WCSERR, 
                      "INTERPOLATION=%s specifies an unsupported interpolation method.",
                      "msWCSGetCoverage()",
                      params->interpolation );
          return msWCSException(map, params->version);
      }
  }
   
  /* apply region and size to map object.  */
  map->width = params->width;
  map->height = params->height;
  map->extent = params->bbox;
 
  map->cellsize = params->resx; /* pick one, MapServer only supports square cells (what about msAdjustExtent here!) */

  msMapComputeGeotransform(map);

  /* Do we need to fake out stuff for rotated support? */ 
  if( map->gt.need_geotransform ) 
      msMapSetFakedExtent( map ); 

  map->projection.gt = map->gt;

  /* check and make sure there is a format, and that it's valid (TODO: make sure in the layer metadata) */
  if(!params->format) {
    msSetError( MS_WCSERR,  "Missing required FORMAT parameter.", "msWCSGetCoverage()" );
    return msWCSException(map, params->version);
  }
  if(msGetOutputFormatIndex(map,params->format) == -1) {
    msSetError( MS_WCSERR,  "Unrecognized value for the FORMAT parameter.", "msWCSGetCoverage()" );
    return msWCSException(map, params->version);
  }

  /* create a temporary outputformat (we likely will need to tweak parts) */
  format = msCloneOutputFormat(msSelectOutputFormat(map,params->format));
  msApplyOutputFormat(&(map->outputformat), format, MS_NOOVERRIDE, MS_NOOVERRIDE, MS_NOOVERRIDE); 
           
  if(!bandlist) { /* build a bandlist (default is ALL bands) */
    bandlist = (char *) malloc(cm.bandcount*30+30 );
    strcpy(bandlist, "1");
    for(i = 1; i < cm.bandcount; i++)
      sprintf(bandlist+strlen(bandlist),",%d", i+1);
  }
 
  msLayerSetProcessingKey(lp, "BANDS", bandlist);
  sprintf(numbands, "%d", countChars(bandlist, ',')+1);
  msSetOutputFormatOption(map->outputformat, "BAND_COUNT", numbands);
               
  /* create the image object  */
  if(!map->outputformat) {
    msSetError(MS_WCSERR, "The map outputformat is missing!", "msWCSGetCoverage()");
    return msWCSException(map, params->version);
  } else if( MS_RENDERER_GD(map->outputformat) ) {
    image = msImageCreateGD(map->width, map->height, map->outputformat, map->web.imagepath, map->web.imageurl);        
    if( image != NULL ) msImageInitGD( image, &map->imagecolor );
  } else if( MS_RENDERER_RAWDATA(map->outputformat) ) {
    image = msImageCreate(map->width, map->height, map->outputformat, map->web.imagepath, map->web.imageurl, map);
  } else {
    msSetError(MS_WCSERR, "Map outputformat not supported for WCS!", "msWCSGetCoverage()");
    return msWCSException(map, params->version);
  }

  /* Actually produce the "grid". */
  status = msDrawRasterLayerLow( map, lp, image );
  if( status != MS_SUCCESS ) {
    return msWCSException(map, params->version);
  }
    
  /* Emit back to client. */
  msIO_printf("Content-type: %s%c%c", MS_IMAGE_MIME_TYPE(map->outputformat), 10,10);
  status = msSaveImage(map, image, NULL);

  if( status != MS_SUCCESS )
  {
      /* unfortunately, the image content type will have already been sent
         but that is hard for us to avoid.  The main error that could happen
         here is a misconfigured tmp directory or running out of space. */
      return msWCSException(map, params->version);
  }

  /* Cleanup */
  msFreeImage(image);
  msApplyOutputFormat(&(map->outputformat), NULL, MS_NOOVERRIDE, MS_NOOVERRIDE, MS_NOOVERRIDE);
  /* msFreeOutputFormat(format); */

  return status;
}
#endif /* def USE_WCS_SVR */

/************************************************************************/
/*                           msWCSDispatch()                            */
/*                                                                      */
/*      Entry point for WCS requests                                    */
/************************************************************************/

int msWCSDispatch(mapObj *map, cgiRequestObj *request)
{
#ifdef USE_WCS_SVR
  wcsParamsObj *params;  

  /* populate the service parameters */
  params = msWCSCreateParams();
  if( msWCSParseRequest(request, params, map) == MS_FAILURE )
  {
      msWCSFreeParams(params); /* clean up */
      free(params);
      return MS_FAILURE;
  }

  /* If SERVICE is specified then it MUST be "WCS" */
  if(params->service && strcasecmp(params->service, "WCS") != 0)
  {
      msWCSFreeParams(params); /* clean up */
      free(params);
      return MS_DONE;
  }

  /* If SERVICE and REQUEST not included then not a WCS request */
  if(!params->service && !params->request)
  {
      msWCSFreeParams(params); /* clean up */
      free(params);
      return MS_DONE;
  }

  /*
  ** ok, it's a WCS request, check what we can at a global level and then dispatch to the various request handlers
  */

  /* version is optional, but we do set a default value of 1.0.0, make sure request isn't for something different */
  if(strcmp(params->version, "1.0.0") != 0) {
    msSetError(MS_WCSERR, "WCS Server does not support VERSION %s.", "msWCSDispatch()", params->version);
    msWCSException(map, params->version);
  
    msWCSFreeParams(params); /* clean up */
    free(params);
    params = NULL;

    return MS_FAILURE;
  }

  /*
  ** Start dispatching requests
  */
  if(strcasecmp(params->request, "GetCapabilities") == 0)    
    return msWCSGetCapabilities(map, params, request);
  else if(strcasecmp(params->request, "DescribeCoverage") == 0)    
    return msWCSDescribeCoverage(map, params);
  else if(strcasecmp(params->request, "GetCoverage") == 0)    
    return msWCSGetCoverage(map, request, params);

  return MS_DONE; /* not a WCS request, let MapServer take it */
#else
  msSetError(MS_WCSERR, "WCS server support is not available.", "msWCSDispatch()");
  return MS_FAILURE;
#endif
}

/************************************************************************/
/*                      msWCSGetCoverageMetadata()                      */
/************************************************************************/

#ifdef USE_WCS_SVR
static int msWCSGetCoverageMetadata( layerObj *layer, coverageMetadataObj *cm )
{
  /* get information that is "data" independent */
  if((cm->srs = msOWSGetEPSGProj(&(layer->projection), &(layer->metadata), "COM", MS_TRUE)) == NULL) {
    if((cm->srs = msOWSGetEPSGProj(&(layer->map->projection), &(layer->map->web.metadata), "COM", MS_TRUE)) == NULL) {
      msSetError(MS_WCSERR, "Unable to determine the SRS for this layer, no projection defined and no metadata available.", "msWCSGetCoverageMetadata()");
      return MS_FAILURE;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      If we have "virtual dataset" metadata on the layer, then use    */
  /*      that in preference to inspecting the file(s).                   */
  /* -------------------------------------------------------------------- */
  if( msOWSLookupMetadata(&(layer->metadata), "COM", "extent") != NULL ) {
    const char *value;

    /* get extent */
    cm->extent.minx = 0.0;
    cm->extent.maxx = 0.0;
    cm->extent.miny = 0.0;
    cm->extent.maxy = 0.0;
    if( msOWSGetLayerExtent( layer->map, layer, "CO", &cm->extent ) == MS_FAILURE )
      return MS_FAILURE;
    
    /* get resolution */
    cm->xresolution = 0.0;
    cm->yresolution = 0.0;
    if( (value = msOWSLookupMetadata(&(layer->metadata), "COM", "resolution")) != NULL ) {
      char **tokens;
      int n;
            
      tokens = split(value, ' ', &n);
      if( tokens == NULL || n != 2 ) {
        msSetError( MS_WCSERR, "Wrong number of arguments for wcs|ows_resolution metadata.", "msWCSGetCoverageMetadata()");
        msFreeCharArray( tokens, n );
        return MS_FAILURE;
      }
      cm->xresolution = atof(tokens[0]);
      cm->yresolution = atof(tokens[1]);
      msFreeCharArray( tokens, n );
    }

    /* get Size (in pixels and lines) */
    cm->xsize = 0;
    cm->ysize = 0;
    if( (value=msOWSLookupMetadata(&(layer->metadata), "COM", "size")) != NULL ) {
      char **tokens;
      int n;
            
      tokens = split(value, ' ', &n);
      if( tokens == NULL || n != 2 ) {
        msSetError( MS_WCSERR, "Wrong number of arguments for wcs|ows_size metadata.", "msWCSGetCoverageDomain()");
        msFreeCharArray( tokens, n );
        return MS_FAILURE;
      }
      cm->xsize = atoi(tokens[0]);
      cm->ysize = atoi(tokens[1]);
      msFreeCharArray( tokens, n );
    }

    /* try to compute raster size */
    if( cm->xsize == 0 && cm->ysize == 0 && cm->xresolution != 0.0 && cm->yresolution != 0.0 && cm->extent.minx != cm->extent.maxx && cm->extent.miny != cm->extent.maxy ) {
      cm->xsize = (int) ((cm->extent.maxx - cm->extent.minx) / cm->xresolution + 0.5);
      cm->ysize = (int) fabs((cm->extent.maxy - cm->extent.miny) / cm->yresolution + 0.5);
    }

    /* try to compute raster resolution */
    if( (cm->xresolution == 0.0 || cm->yresolution == 0.0) && cm->xsize != 0 && cm->ysize != 0 ) {
      cm->xresolution = (cm->extent.maxx - cm->extent.minx) / cm->xsize;
      cm->yresolution = (cm->extent.maxy - cm->extent.miny) / cm->ysize;
    }

    /* do we have information to do anything */
    if( cm->xresolution == 0.0 || cm->yresolution == 0.0 || cm->xsize == 0 || cm->ysize == 0 ) {
      msSetError( MS_WCSERR, "Failed to collect extent and resolution for WCS coverage from metadata.  Need value wcs|ows_resolution or wcs|ows_size values.", "msWCSGetCoverageMetadata()");
      return MS_FAILURE;
    }
        
    /* compute geotransform */
    cm->geotransform[0] = cm->extent.minx;
    cm->geotransform[1] = cm->xresolution;
    cm->geotransform[2] = 0.0;
    cm->geotransform[3] = cm->extent.maxy;
    cm->geotransform[4] = 0.0;
    cm->geotransform[5] = -fabs(cm->yresolution);

    /* get bands count, or assume 1 if not found */
    cm->bandcount = 1;
    if( (value=msOWSLookupMetadata(&(layer->metadata), "COM", "bandcount")) != NULL) {
      cm->bandcount = atoi(value);
    }

    /* get bands type, or assume float if not found */
    cm->imagemode = MS_IMAGEMODE_FLOAT32;
    if( (value=msOWSLookupMetadata(&(layer->metadata), "COM", "imagemode")) != NULL ) {
      if( EQUAL(value,"INT16") )
        cm->imagemode = MS_IMAGEMODE_INT16;
      else if( EQUAL(value,"FLOAT32") )
        cm->imagemode = MS_IMAGEMODE_FLOAT32;
      else if( EQUAL(value,"BYTE") )
        cm->imagemode = MS_IMAGEMODE_BYTE;
      else {
        msSetError( MS_WCSERR, "Content of wcs|ows_imagemode (%s) not recognised.  Should be one of PC256 (byte), INT16 or FLOAT32.", "msWCSGetCoverageMetadata()", value );
        return MS_FAILURE;
      }
    }
  } else if( layer->data == NULL ) { /* no virtual metadata, not ok unless we're talking 1 image, hopefully we can fix that */
    msSetError( MS_WMSERR, "RASTER Layer with no DATA statement and no WCS virtual dataset metadata.  Tileindexed raster layers not supported for WCS without virtual dataset metadata (cm->extent, wcs_res, wcs_size).", "msWCSGetCoverageDomain()" );
    return MS_FAILURE;
  } else { /* work from the file (e.g. DATA) */
    GDALDatasetH hDS;
    GDALRasterBandH hBand;
    char szPath[MS_MAXPATHLEN];
  
    msGDALInitialize();

    msAcquireLock( TLOCK_GDAL );
    hDS = GDALOpen(msBuildPath3(szPath,  layer->map->mappath, layer->map->shapepath, layer->data) , GA_ReadOnly );
    if( hDS == NULL ) {
      msReleaseLock( TLOCK_GDAL );
      msSetError( MS_IOERR, "%s", "msWCSGetCoverageMetadata()", CPLGetLastErrorMsg() );
      return MS_FAILURE;
    }


    msGetGDALGeoTransform( hDS, layer->map, layer, cm->geotransform );

    cm->xsize = GDALGetRasterXSize( hDS );
    cm->ysize = GDALGetRasterYSize( hDS );

    cm->extent.minx = cm->geotransform[0];
    cm->extent.maxx = cm->geotransform[0] + cm->geotransform[1] * cm->xsize + cm->geotransform[2] * cm->ysize;
    cm->extent.miny = cm->geotransform[3] + cm->geotransform[4] * cm->xsize + cm->geotransform[5] * cm->ysize;
    cm->extent.maxy = cm->geotransform[3];
    
    /* TODO: need to set resolution */
    
    cm->bandcount = GDALGetRasterCount( hDS );
        
    if( cm->bandcount == 0 ) {
      msReleaseLock( TLOCK_GDAL );
      msSetError( MS_WMSERR, "Raster file %s has no raster bands.  This cannot be used in a layer.", "msWCSGetCoverageMetadata()", layer->data );
      return MS_FAILURE;
    }

    hBand = GDALGetRasterBand( hDS, 1 );
    switch( GDALGetRasterDataType( hBand ) ) {
    case GDT_Byte:
      cm->imagemode = MS_IMAGEMODE_BYTE;
      break;
    case GDT_Int16:
      cm->imagemode = MS_IMAGEMODE_INT16;
      break;
    default:
      cm->imagemode = MS_IMAGEMODE_FLOAT32;
      break;
    }

    GDALClose( hDS );
    msReleaseLock( TLOCK_GDAL );
  }
 
  /* we must have the bounding box in lat/lon [WGS84(DD)/EPSG:4326] */
  cm->llextent = cm->extent;
  
  /* Already in latlong .. use directly. */
  if( layer->projection.proj != NULL && pj_is_latlong(layer->projection.proj))
  {
      /* no change */
  }

  else if (layer->projection.numargs > 0 && !pj_is_latlong(layer->projection.proj)) /* check the layer projection */
    msProjectRect(&(layer->projection), NULL, &(cm->llextent));

  else if (layer->map->projection.numargs > 0 && !pj_is_latlong(layer->map->projection.proj)) /* check the map projection */
    msProjectRect(&(layer->map->projection), NULL, &(cm->llextent));

  else { /* projection was specified in the metadata only (EPSG:... only at the moment)  */
    projectionObj proj;
    char projstring[32];

    msInitProjection(&proj); /* or bad things happen */

    sprintf(projstring, "init=epsg:%.20s", cm->srs+5);
    if (msLoadProjectionString(&proj, projstring) != 0) return MS_FAILURE;
    msProjectRect(&proj, NULL, &(cm->llextent));    
  }

  return MS_SUCCESS;
}
#endif /* def USE_WCS_SVR */


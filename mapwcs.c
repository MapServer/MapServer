#include "map.h"
#include "maperror.h"
#include "gdal.h"
#include "mapthread.h"
#include "cpl_string.h" // GDAL string handling

#ifdef USE_WCS_SVR

typedef struct {
  // TODO: union?
} rangeObj; 

typedef struct {
  char *version;		// 1.0.0 for now
  char *request;		// GetCapabilities|DescribeCoverage|GetCoverage
  char *service;		// MUST be WCS
  char *section;		// of capabilities document: Service|Capability|ContentMetadata
  char **coverages;		// NULL terminated list of coverages (in the case of a GetCoverage there will only be 1)
  char *crs;			// request coordinate reference system
  char *response_crs;	// response coordinate reference system
  rectObj bbox;		    // subset bounding box (3D), although we'll only use 2D
  char **times;
  long width, height, depth;	// image dimensions
  double resx, resy, resz;      // resolution
  char *format;
  char *exceptions;		// exception MIME type, (default application=vnd.ogc.se_xml?)
} wcsParamsObj;

static int 
msWCSGetCoverageDomain( mapObj *map, layerObj *layer, 
                        double *padfGeoTransform, 
                        int *pnXSize, int *pnYSize, int *pnBandCount, 
                        int *pnDataType
                        /* what about temporal? */ );

static int msWCSException(mapObj *map, const char *version) 
{
  printf("Content-type: text/xml%c%c",10,10);

  printf("<ServiceExceptionReport\n");
  printf("xmlns=\"http://www.opengis.net/ogc\" ");
  printf("xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ");
  printf("xsi:schemaLocation=\"http://www.opengis.net/ogc http://ogc.dmsolutions.ca/wfs/1.0/OGC-exception.xsd\">\n");
  printf("  <ServiceException>\n");
  msWriteErrorXML(stdout);
  printf("  </ServiceException>\n");
  printf("</ServiceExceptionReport>\n");

  return MS_FAILURE;
}

static wcsParamsObj *msWCSCreateParams()
{
  wcsParamsObj *params;

  fprintf(stderr, "in msWCSCreateParams()\n");
  
  params = (wcsParamsObj *) calloc(1, sizeof(wcsParamsObj));
  if(params) { // initialize a few things to default values
    params->version = strdup("1.0.0");
  }
  
  return params;
}

static void msWCSFreeParams(wcsParamsObj *params)
{
  fprintf(stderr, "in msWCSFreeParams()\n");

  if(params) {
    // TODO
    if(params->version) free(params->version);
    if(params->request) free(params->request);
    if(params->service) free(params->service);
    if(params->section) free(params->section);
    if(params->crs) free(params->crs);
    if(params->response_crs) free(params->crs);      
    if(params->format) free(params->format);
    if(params->exceptions) free(params->exceptions);
  }
  
  fprintf(stderr, "done msWCSFreeParams()\n");
}

static int msWCSIsLayerSupported(layerObj *lp)
{
    // only raster layers, with 'DUMP TRUE' explicitly defined, are elligible to be served via WCS, WMS rasters are not ok
    if(lp->dump && (lp->type == MS_LAYER_RASTER) && lp->connectiontype != MS_WMS) return MS_TRUE;

    return MS_FALSE;
}


static int msWCSParseRequest(cgiRequestObj *request, wcsParamsObj *params, mapObj *map)
{
  int i;

  if(!request || !params) // nothing to do
	return MS_SUCCESS;

  fprintf(stderr, "in msWCSParseRequest()\n");	

  if(request->NumParams > 0) {
    for(i=0; i<request->NumParams; i++) {
    
       if(strcasecmp(request->ParamNames[i], "VERSION") == 0)
	     params->version = strdup(request->ParamValues[i]);
       else if(strcasecmp(request->ParamNames[i], "REQUEST") == 0)
	     params->request = strdup(request->ParamValues[i]);
       else if(strcasecmp(request->ParamNames[i], "SERVICE") == 0)
	     params->service = strdup(request->ParamValues[i]);
	   else if(strcasecmp(request->ParamNames[i], "SECTION") == 0)
	     params->section = strdup(request->ParamValues[i]);

       // GetCoverage parameters.
       else if(strcasecmp(request->ParamNames[i], "BBOX") == 0) {
         char **tokens;
         int n;
          
         tokens = split(request->ParamValues[i], ',', &n);
         if(tokens==NULL || n != 4) {
           msSetError(MS_WMSERR, "Wrong number of arguments for BBOX.", "msWMSLoadGetMapParams()");
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

       // and so on...
    }
  }

  fprintf(stderr, "version=%s\n", params->version);

  // we are not dealing with an XML encoded request at this point
  return MS_SUCCESS;
}

static int msWCSGetCapabilities_Service(mapObj *map, wcsParamsObj *params)
{
  char *script_url=NULL, *script_url_encoded;

  // we need this server's onlineresource.
  if((script_url=msOWSGetOnlineResource(map, "wcs_onlineresource")) == NULL || (script_url_encoded = msEncodeHTMLEntities(script_url)) == NULL)
    return msWCSException(map, params->version);

  // start the services section
  printf("<Service>\n");
  printf("  <Name>MapServer WCS</Name>\n");

  printf("  <OnlineResource>%s</OnlineResource>\n", script_url_encoded);

  // done
  printf("</Service>\n\n");

  // clean up
  free(script_url);
  free(script_url_encoded);

  return MS_SUCCESS;
}

static int msWCSGetCapabilities_Capability(mapObj *map, wcsParamsObj *params)
{
  // start the capabilties section
  printf("<Capability>\n");

  // done
  printf("</Capability>\n");

  return MS_SUCCESS;
}

static int msWCSGetCapabilities_ContentMetadata(mapObj *map, wcsParamsObj *params)
{
  return MS_SUCCESS;
}

static int msWCSGetCapabilities(mapObj *map, wcsParamsObj *params)
{

  printf("Content-type: text/xml%c%c",10,10);

  // print common capability elements 
  msOWSPrintMetadata(stdout, map->web.metadata, "wcs_encoding", OWS_NOERR, "<?xml version='1.0' encoding=\"%s\" ?>\n", "ISO-8859-1");

  printf("<WCS_Capabilities\n"
         "   version=\"%s\" \n"
         "   updateSequence=\"0\" \n"
         "   xmlns=\"http://www.opengis.net/wcs\" \n"
         "   xmlns:ogc=\"http://www.opengis.net/ogc\" \n"
         "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
         "   xsi:schemaLocation=\"http://schemas.opengis.net/wcs/%s/wcsCapabilities.xsd\">\n", params->version, params->version);
         
  // print the various capability sections
  if(!params->section || strcasecmp(params->section, "Service"))
    msWCSGetCapabilities_Service(map, params);

  if(!params->section || strcasecmp(params->section, "Capability"))
    msWCSGetCapabilities_Capability(map, params);

  if(!params->section || strcasecmp(params->section, "ContentMetadata"))
    msWCSGetCapabilities_ContentMetadata(map, params);

  // done
  printf("</WCS_Capabilities>\n");

  // clean up
  // msWCSFreeParams(params);

  return MS_SUCCESS;
}

/* Example RectifiedGrid from GML 3.0 spec 

<gml:RectifiedGrid dimension="2"> 
    <gml:limits> 
      <gml:GridEnvelope> 
        <gml:low>1 1</gml:low> 
        <gml:high>3 3</gml:high> 
      </gml:GridEnvelope> 
    </gml:limits> 
    <gml:axisName>u</gml:axisName> 
    <gml:axisName>v</gml:axisName> 
    <gml:origin> 
      <gml:Point gml:id="palindrome"> 
        <gml:coordinates>1.2,3.3,2.1</gml:coordinates> 
      </gml:Point> 
    </gml:origin> 
    <gml:offsetVector>1,2,3</gml:offsetVector> 
    <gml:offsetVector >2,1,0</gml:offsetVector> 
  </gml:RectifiedGrid>   
*/

static int msWCSDescribeCoverage(mapObj *map, wcsParamsObj *params)
{
  return MS_SUCCESS;
}

static int msWCSGetCoverage(mapObj *map, wcsParamsObj *params)
{
    imageObj   *image;
    layerObj   *layer;
    int         status, i;

    // Did we get BBOX values?  

    if( fabs((params->bbox.maxx - params->bbox.minx)) < 0.000000000001 
        || fabs(params->bbox.maxy - params->bbox.miny) < 0.000000000001 )
    {
        msSetError( MS_WMSERR, 
                    "BBOX missing or specifies an empty region.", 
                    "msWCSGetCoverage()" );
        return msWCSException(map, params->version);
    }
    
    // Compute width/height from bbox and cellsize. 

    if( (params->resx == 0.0 || params->resy == 0.0)
        && params->width != 0 && params->height != 0 )
    {
        params->resx = (params->bbox.maxx - params->bbox.minx) / params->width;
        params->resy = (params->bbox.maxy - params->bbox.miny) / params->height;
    }
    
    // Compute cellsize/res from bbox and raster size.

    if( (params->width == 0 || params->height == 0)
        && params->resx != 0 && params->resy != 0 )
    {
        params->width = (int) ((params->bbox.maxx - params->bbox.minx) / params->resx + 0.5);
        params->height = (int) ((params->bbox.maxy - params->bbox.miny) / params->resy + 0.5);
    }

    // Are we still underspecified? 

    if( params->width == 0 || params->height == 0 
        || params->resx == 0.0 || params->resy == 0.0 )
    {
        msSetError( MS_WMSERR, 
                    "A nonzero RESX/RESY or WIDTH/HEIGHT is required but neither was provided.", 
                    "msWCSGetCoverage()" );
        return msWCSException(map, params->version);
    }
    
    // Apply region and size to map object. 

    map->width = params->width;
    map->height = params->height;
    map->extent = params->bbox;
 
    map->cellsize = params->resx; // pick on, MapServer only supports square cells

    // Should we compute a new extent, ala WMS?
    if( fabs(params->resx/params->resy - 1.0) > 0.001 )
    {
        msSetError( MS_WMSERR, 
                    "RESX and RESY don't match.  This is currently not a supported option for MapServer WCS.", 
                    "msWCSGetCoverage()" );
        return msWCSException(map, params->version);
    }
                        
    // Find the layer we are working with. 
    layer = NULL;
    for( i = 0; i < map->numlayers; i++ )
    {
        if( EQUAL(map->layers[i].name,params->coverages[0]) )
        {
            layer = map->layers + i;
            break;
        }
    }

    if( layer == NULL )
    {
        msSetError( MS_WMSERR, 
                    "COVERAGE=%s not found, not in supported layer list.", 
                    "msWCSGetCoverage()", params->coverages[0] );
        return msWCSException(map, params->version);
    }

    // Select format into map object.

    /* for now we use the default output format defined in the .map file */

    // Create the image object. 
    if(!map->outputformat) {
        msSetError(MS_GDERR, "Map outputformat not set!", 
                   "msWCSGetCoverage()");
        return msWCSException(map, params->version);
    }
    else if( MS_RENDERER_GD(map->outputformat) )
    {
        image = msImageCreateGD(map->width, map->height, map->outputformat, 
				map->web.imagepath, map->web.imageurl);        
        if( image != NULL ) msImageInitGD( image, &map->imagecolor );
    }
    else if( MS_RENDERER_RAWDATA(map->outputformat) )
    {
        image = msImageCreate(map->width, map->height, map->outputformat,
                              map->web.imagepath, map->web.imageurl);
    }
    else
    {
        msSetError(MS_GDERR, "Map outputformat not supported for WCS!", 
                   "msWCSGetCoverage()");
        return msWCSException(map, params->version);
    }

    // Actually produce the "grid".
    status = msDrawRasterLayerLow( map, layer, image );
    if( status != MS_SUCCESS )
    {
        return status;
    }
    
    // Emit back to client.
    printf("Content-type: %s%c%c", MS_IMAGE_MIME_TYPE(map->outputformat), 10,10);
    status = msSaveImage(map, image, NULL);

    // Cleanup
    msFreeImage(image);

    return status;
}
#endif /* def USE_WCS_SVR */

// Entry point for WCS requests
int msWCSDispatch(mapObj *map, cgiRequestObj *request)
{
#ifdef USE_WCS_SVR
  wcsParamsObj *params;
  
  fprintf(stderr, "in msWCSDispatch()\n");

  // populate the service parameters
  params = msWCSCreateParams();
  if( msWCSParseRequest(request, params, map) == MS_FAILURE )
      return MS_FAILURE;

  // If SERVICE is specified then it MUST be "WCS"
  if(params->service && strcasecmp(params->service, "WCS") != 0)
    return MS_DONE;

  // If SERVICE and REQUEST not included then not a WCS request
  if(!params->service && !params->request)
    return MS_DONE;

  /*
  ** Start dispatching requests
  */
  if(strcasecmp(params->request, "GetCapabilities") == 0)    
    return msWCSGetCapabilities(map, params);
  else if(strcasecmp(params->request, "GetCoverage") == 0)    
    return msWCSGetCoverage(map, params);

  return MS_DONE; // not a WCS request, let MapServer take it
#else
  msSetError(MS_WCSERR, "WCS server support is not available.", "msWCSDispatch()");
  return MS_FAILURE;
#endif
}

/************************************************************************/
/*                      msWCSGetCoverageDomain()                        */
/************************************************************************/

#ifdef USE_WCS_SVR
static int 
msWCSGetCoverageDomain( mapObj *map, layerObj *layer, 
                        double *padfGeoTransform, 
                        int *pnXSize, int *pnYSize, int *pnBandCount, 
                        int *pnPreferredImageMode
                        /* what about temporal? */ )

{
/* -------------------------------------------------------------------- */
/*      If we have "virtual dataset" metadata on the layer, then use    */
/*      that in preference to inspecting the file(s).                   */
/* -------------------------------------------------------------------- */
    if( msLookupHashTable(layer->metadata, "wcs_extent") != NULL )
    {
        rectObj wcs_extent;
        double wcs_res[2];
        char *value;

        /* get Extent */
        wcs_extent.minx = 0.0;
        wcs_extent.maxx = 0.0;
        wcs_extent.miny = 0.0;
        wcs_extent.maxx = 0.0;
        if( msOWSGetLayerExtent( map, layer, &wcs_extent ) == MS_FAILURE )
            return MS_FAILURE;
        
        /* get Resolution */
        wcs_res[0] = 0.0;
        wcs_res[1] = 0.0;
        if( (value=msLookupHashTable(layer->metadata,"wcs_res")) != NULL )
        {
            char **tokens;
            int n;
            
            tokens = split(value, ' ', &n);
            if( tokens == NULL || n != 2 )
            {
                msSetError( MS_WMSERR, 
                            "Wrong number of arguments for wcs_res metadata.",
                            "msWCSGetCoverageDomain()");
                msFreeCharArray( tokens, n );
                return MS_FAILURE;
            }
            wcs_res[0] = atof(tokens[0]);
            wcs_res[1] = atof(tokens[1]);
            msFreeCharArray( tokens, n );
        }
        
        /* get Size (in pixels and lines) */
        *pnXSize = 0;
        *pnYSize = 0;
        if( (value=msLookupHashTable(layer->metadata,"wcs_size")) != NULL )
        {
            char **tokens;
            int n;
            
            tokens = split(value, ' ', &n);
            if( tokens == NULL || n != 2 )
            {
                msSetError( MS_WMSERR, 
                            "Wrong number of arguments for wcs_size metadata.",
                            "msWCSGetCoverageDomain()");
                msFreeCharArray( tokens, n );
                return MS_FAILURE;
            }
            *pnXSize = atoi(tokens[0]);
            *pnYSize = atoi(tokens[1]);
            msFreeCharArray( tokens, n );
        }

        /* try to compute raster size */
        if( *pnXSize == 0 && *pnYSize == 0 
            && wcs_res[0] != 0.0 && wcs_res[1] != 0.0
            && wcs_extent.minx != wcs_extent.maxx
            && wcs_extent.miny != wcs_extent.maxy )
        {
            *pnXSize = (int) ((wcs_extent.maxx - wcs_extent.minx) 
                              / wcs_res[0] + 0.5);
            *pnYSize = (int) fabs((wcs_extent.maxy - wcs_extent.miny) 
                                  / wcs_res[1] + 0.5);
        }

        /* try to compute raster resolution */
        if( (wcs_res[0] == 0.0 || wcs_res[1] == 0.0)
            && *pnXSize != 0 && *pnYSize != 0 )
        {
            wcs_res[0] = (wcs_extent.maxx - wcs_extent.minx) / *pnXSize;
            wcs_res[1] = (wcs_extent.maxy - wcs_extent.miny) / *pnYSize;
        }

        /* Check if defined */
        if( wcs_res[0] == 0.0 || wcs_res[1] == 0.0 
            || *pnXSize == 0 || *pnYSize == 0 )
        {
            msSetError( MS_WMSERR, 
                        "Failed to collect extent and resolution for WCS coverage from metadata.  Need value wcs_res or wcs_size values.",
                        "msWCSGetCoverageDomain()");
            return MS_FAILURE;
        }
        
        /* compute geotransform */
        padfGeoTransform[0] = wcs_extent.minx;
        padfGeoTransform[1] = wcs_res[0];
        padfGeoTransform[2] = 0.0;
        padfGeoTransform[3] = wcs_extent.maxy;
        padfGeoTransform[4] = 0.0;
        padfGeoTransform[5] = -fabs(wcs_res[1]);

        /* Get bands count, or assume 1 if not found. */
        *pnBandCount = 1;
        if( (value=msLookupHashTable(layer->metadata,"wcs_bandcount")) != NULL)
        {
            *pnBandCount = atoi(value);
        }

        /* Get bands type, or assume float if not found.o */
        *pnPreferredImageMode = MS_IMAGEMODE_FLOAT32;
        if( (value=msLookupHashTable(layer->metadata,"wcs_imagemode")) != NULL )
        {
            if( EQUAL(value,"INT16") )
                *pnPreferredImageMode = MS_IMAGEMODE_INT16;
            else if( EQUAL(value,"FLOAT32") )
                *pnPreferredImageMode = MS_IMAGEMODE_FLOAT32;
            else if( EQUAL(value,"BYTE") )
                *pnPreferredImageMode = MS_IMAGEMODE_BYTE;
            else
            {
                msSetError( MS_WMSERR, 
                            "Content of wcs_imagemode (%s) not recognised.  Should be one of PC256 (byte), INT16 or FLOAT32.", 
                            "msWCSGetCoverageDomain()", value );
                return MS_FAILURE;
            }
        }
        return MS_SUCCESS;
    }
    
/* -------------------------------------------------------------------- */
/*      We require virtual dataset information for tileindexed          */
/*      layers.  If we don't have it, dump out an error.                */
/* -------------------------------------------------------------------- */
    else if( layer->data == NULL )
    {
        msSetError( MS_WMSERR, 
                    "RASTER Layer with no DATA statement and no WCS virtual dataset metadata.  Tileindexed raster layers not supported for WCS without virtual dataset metadata (wcs_extent, wcs_res, wcs_size).",
                    "msWCSGetCoverageDomain()" );
        return MS_FAILURE;
    }

/* -------------------------------------------------------------------- */
/*      Otherwise open the target file to get information.              */
/* -------------------------------------------------------------------- */
    else 
    {
        GDALDatasetH hDS;
        GDALRasterBandH hBand;
        char szPath[MS_MAXPATHLEN];
  
        msGDALInitialize();

        msAcquireLock( TLOCK_GDAL );
        hDS = GDALOpen(msBuildPath3(szPath,  map->mappath, map->shapepath, 
                                    layer->data) , GA_ReadOnly );
        if( hDS == NULL )
        {
            msReleaseLock( TLOCK_GDAL );
            msSetError( MS_IOERR, "%s", 
                        "msWCSGetCoverageDomain()", 
                        CPLGetLastErrorMsg() );
            return MS_FAILURE;
        }

        msGetGDALGeoTransform( hDS, map, layer, padfGeoTransform );
        *pnXSize = GDALGetRasterXSize( hDS );
        *pnYSize = GDALGetRasterYSize( hDS );

        *pnBandCount = GDALGetRasterCount( hDS );
        
        if( *pnBandCount == 0 )
        {
            msReleaseLock( TLOCK_GDAL );
            msSetError( MS_WMSERR, 
                        "Raster file %s has no raster bands.  This cannot be used in a layer.",
                    "msWCSGetCoverageDomain()", layer->data );
            return MS_FAILURE;
        }

        hBand = GDALGetRasterBand( hDS, 1 );
        switch( GDALGetRasterDataType( hBand ) )
        {
          case GDT_Byte:
            *pnPreferredImageMode = MS_IMAGEMODE_BYTE;
            break;

          case GDT_Int16:
            *pnPreferredImageMode = MS_IMAGEMODE_INT16;
            break;

          default:
            *pnPreferredImageMode = MS_IMAGEMODE_FLOAT32;
            break;
        }

        GDALClose( hDS );
        msReleaseLock( TLOCK_GDAL );

        return MS_SUCCESS;
    }
}
#endif

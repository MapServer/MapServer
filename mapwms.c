/* $Id$ */

#include "map.h"
#include "maperror.h"
#include "mapgml.h"
#include "gdfonts.h"

#include "maptemplate.h"

#include <stdarg.h>
#include <time.h>
#include <string.h>

/*
** msWMSGetEPSGProj() moved to mapproject.c and renamed msGetEPSGProj(). This function turns
** out to be generally useful outside of WMS context.
*/

/* ==================================================================
 * WMS Server stuff.
 * ================================================================== */
#ifdef USE_WMS

static int printMetadata(hashTableObj metadata, const char *name, 
                         int action_if_not_found, const char *format, 
                         const char *default_value);
// WMS_NOERR and WMS_WARN passed as action_if_not_found to printMetadata()
#define WMS_NOERR   0
#define WMS_WARN    1


/*
** msWMSException()
**
** Report current MapServer error in requested format.
*/
static char *wms_exception_format=NULL;

int msWMSException(mapObj *map, const char *wmtversion) 
{
  // Establish default exception format depending on VERSION
  if (wms_exception_format == NULL) 
  {
      if (wmtversion && strcasecmp(wmtversion, "1.0.0") <= 0)
          wms_exception_format = "INIMAGE";  // WMS 1.0.0
      else if (wmtversion && strcasecmp(wmtversion, "1.0.7") <= 0)
          wms_exception_format = "SE_XML";   // WMS 1.0.1 to 1.0.7
      else
          wms_exception_format = "application/vnd.ogc.se_xml"; // WMS 1.0.8, 1.1.0 and later
  }

  if (strcasecmp(wms_exception_format, "INIMAGE") == 0 ||
      strcasecmp(wms_exception_format, "BLANK") == 0 ||
      strcasecmp(wms_exception_format, "application/vnd.ogc.se_inimage")== 0 ||
      strcasecmp(wms_exception_format, "application/vnd.ogc.se_blank") == 0)
  {
    gdFontPtr font = gdFontSmall;
    gdImagePtr img=NULL;
    int width=400, height=300, color;
    int nMargin =5;
    int nTextLength = 0;
    int nUsableWidth = 0;
    int nMaxCharsPerLine = 0;
    int nLines = 0;
    int i = 0;
    int nStart = 0;
    int nEnd = 0;
    int nLength = 0;
    char **papszLines = NULL;
    int nXPos = 0;
    int nYPos = 0;
    int nWidthTxt = 0;
    int nSpaceBewteenLines = font->h;
    int nBlack = 0;     
    outputFormatObj *format = NULL;

    if (map) {
      width = map->width;
      height = map->height;
      format = map->outputformat;
    }

    if( format == NULL )
        format = msCreateDefaultOutputFormat( NULL, "image/gif" );
    if( format == NULL )
        format = msCreateDefaultOutputFormat( NULL, "image/png" );

    img = gdImageCreate(width, height);
    color = gdImageColorAllocate(img, 255,255,255);  // BG color
    nBlack = gdImageColorAllocate(img, 0,0,0);        // Text color

    if (strcasecmp(wms_exception_format, "BLANK") != 0 &&
        strcasecmp(wms_exception_format, "application/vnd.ogc.se_blank") != 0)
    {
      char errormsg[MESSAGELENGTH+ROUTINELENGTH+4];
      errorObj *ms_error = msGetErrorObj();
      sprintf(errormsg, "%s: %s", ms_error->routine, ms_error->message);
      nTextLength = strlen(errormsg); 
      nWidthTxt  =  nTextLength * font->w;
      nUsableWidth = width - (nMargin*2);
/* -------------------------------------------------------------------- */
/*      Check to see if it all fits on one line. If not, split the      */
/*      text on sevral lines.                                           */
/* -------------------------------------------------------------------- */
    if (nWidthTxt > nUsableWidth)
    {
        nMaxCharsPerLine =  nUsableWidth/font->w;
        nLines = (int) ceil ((double)nTextLength / (double)nMaxCharsPerLine);
        if (nLines > 0)
        {
            papszLines = (char **)malloc(nLines*sizeof(char *));
            for (i=0; i<nLines; i++)
            {
                papszLines[i] = 
                    (char *)malloc((nMaxCharsPerLine+1)*sizeof(char));
                papszLines[i][0] = '0';
            }
        }
        for (i=0; i<nLines; i++)
        {
            nStart = i*nMaxCharsPerLine;
            nEnd = nStart + nMaxCharsPerLine;
            if (nStart < nTextLength)
            {
                if (nEnd > nTextLength)
                    nEnd = nTextLength;
                nLength = nEnd-nStart;

                memcpy(papszLines[i],errormsg+nStart, nLength);
                papszLines[i][nLength+1] = '0';
            }
        }
    }
    else
    {
        nLines = 1;
        papszLines = (char **)malloc(nLines*sizeof(char *));
        papszLines[0] = (char *)malloc((strlen(errormsg)+1)*sizeof(char));
        papszLines[0] = strcpy(papszLines[0], errormsg);
        papszLines[0][strlen(papszLines[0])+1]='\0';
    }   
    for (i=0; i<nLines; i++)
    {
        nYPos = (nSpaceBewteenLines) * ((i*2) +1); 
        nXPos = nSpaceBewteenLines;

        gdImageString(img, font, nXPos, nYPos,  
                      (unsigned char *)papszLines[i], nBlack);
    }
    if (papszLines)
    {
	for (i=0; i<nLines; i++)
	{
	    free(papszLines[i]);
	}
	free (papszLines);
    }
    //gdImageString(img, font, 5, height/2, errormsg, color);
    }

    printf("Content-type: %s%c%c", MS_IMAGE_MIME_TYPE(format), 10,10);
    msSaveImageGD(img, NULL, format);
    gdImageDestroy(img);

    if( format->refcount == 0 )
        msFreeOutputFormat( format );

  }
  else if (strcasecmp(wms_exception_format, "WMS_XML") == 0) // Only in V1.0.0 
  {
    printf("Content-type: text/xml%c%c",10,10);
    printf("<WMTException version=\"1.0.0\">\n");
    msWriteError(stdout);
    printf("</WMTException>\n");
  }
  else // XML error, the default: SE_XML (1.0.1 to 1.0.7)
       // or application/vnd.ogc.se_xml (1.0.8, 1.1.0 and later)
  {
    if (wmtversion && strcasecmp(wmtversion, "1.0.7") <= 0)
    {
      // In V1.0.1 to 1.0.7, the MIME type was text/xml
      printf("Content-type: text/xml%c%c",10,10);

      printMetadata(map->web.metadata, "wms_encoding", WMS_NOERR,
                "<?xml version='1.0' encoding=\"%s\" standalone=\"no\" ?>\n",
                    "ISO-8859-1");
      printf("<!DOCTYPE ServiceExceptionReport SYSTEM \"http://www.digitalearth.gov/wmt/xml/exception_1_0_1.dtd\">\n");

      printf("<ServiceExceptionReport version=\"1.0.1\">\n");
    }
    else
    {
      // In V1.0.8, 1.1.0 and later, we have OGC-specific MIME types
      // we cannot return anything else than application/vnd.ogc.se_xml here.
      printf("Content-type: application/vnd.ogc.se_xml%c%c",10,10);

      printMetadata(map->web.metadata, "wms_encoding", WMS_NOERR,
                "<?xml version='1.0' encoding=\"%s\" standalone=\"no\" ?>\n",
                    "ISO-8859-1");
      printf("<!DOCTYPE ServiceExceptionReport SYSTEM \"http://www.digitalearth.gov/wmt/xml/exception_1_1_0.dtd\">\n");

      printf("<ServiceExceptionReport version=\"1.1.0\">\n");        
    }

    printf("<ServiceException>\n");
    msWriteError(stdout);
    printf("</ServiceException>\n");
    printf("</ServiceExceptionReport>\n");
  }

  return MS_FAILURE; // so that we can call 'return msWMSException();' anywhere
}

/*
** msRenameLayer()
*/
static int msRenameLayer(layerObj *lp, int count)
{
    char *newname;
    newname = (char*)malloc((strlen(lp->name)+5)*sizeof(char));
    if (!newname) 
    {
        msSetError(MS_MEMERR, NULL, "msWMSDispatch");
        return MS_FAILURE;
    }
    sprintf(newname, "%s_%2.2d", lp->name, count);
    free(lp->name);
    lp->name = newname;
    
    return MS_SUCCESS;
}

/*
** msWMSMakeAllLayersUnique()
*/
static int msWMSMakeAllLayersUnique(mapObj *map, const char *wmtver)
{
  int i, j;

  // Make sure all layers in the map file have valid and unique names
  for(i=0; i<map->numlayers; i++)
  {
      int count=1;
      for(j=i+1; j<map->numlayers; j++)
      {
          if (map->layers[i].name == NULL || map->layers[j].name == NULL)
          {
              msSetError(MS_MISCERR, 
                         "At least one layer is missing a name in map file.", 
                         "msWMSDispatch");
              return msWMSException(map, wmtver);
          }
          if (strcasecmp(map->layers[i].name, map->layers[j].name) == 0 &&
              msRenameLayer(&(map->layers[j]), ++count) != MS_SUCCESS)
          {
              return msWMSException(map, wmtver);
          }
      }

      // Don't forget to rename the first layer if duplicates were found
      if (count > 1 && msRenameLayer(&(map->layers[i]), 1) != MS_SUCCESS)
      {
          return msWMSException(map, wmtver);
      }
  }
  return MS_SUCCESS;
}

/*
**
*/
int msWMSLoadGetMapParams(mapObj *map, const char *wmtver,
                          char **names, char **values, int numentries)
{
  int i, adjust_extent = MS_FALSE;
  int iUnits = -1;
  int nLayerOrder = 0;
  int transparent = MS_NOOVERRIDE;
  outputFormatObj *format = NULL;
   
  // Some of the getMap parameters are actually required depending on the 
  // request, but for now we assume all are optional and the map file 
  // defaults will apply.
  for(i=0; map && i<numentries; i++)
  {
    // getMap parameters
    if (strcasecmp(names[i], "LAYERS") == 0) 
    {
      char **layers;
      int numlayers, j, k, iLayer;

      layers = split(values[i], ',', &numlayers);
      if (layers==NULL || numlayers < 1) {
        msSetError(MS_MISCERR, "At least one layer name required in LAYERS.",
                   "msWMSLoadGetMapParams()");
        return msWMSException(map, wmtver);
      }

      for (iLayer=0; iLayer < map->numlayers; iLayer++)
          map->layerorder[iLayer] = iLayer;   

      for(j=0; j<map->numlayers; j++) 
      {
        // Keep only layers with status=DEFAULT by default
        // Layer with status DEFAULT is drawn first.
        if (map->layers[j].status != MS_DEFAULT)
           map->layers[j].status = MS_OFF;
        else
           map->layerorder[nLayerOrder++] = j;
      }
       
      for (k=0; k<numlayers; k++)
         for (j=0; j<map->numlayers; j++)
         {         
            // Turn on selected layers only.
            if ((map->layers[j].name && strcasecmp(map->layers[j].name, layers[k]) == 0) ||
                (map->name && strcasecmp(map->name, layers[k]) == 0) ||
                (map->layers[j].group && strcasecmp(map->layers[j].group, layers[k]) == 0))
            {
               map->layers[j].status = MS_ON;
               map->layerorder[nLayerOrder++] = j;
            }
         }
       
      // set all layers with status off at end of array
      for (j=0; j<map->numlayers; j++)
      {
         if (map->layers[j].status == MS_OFF)
           map->layerorder[nLayerOrder++] = j;
      }
       
      msFreeCharArray(layers, numlayers);
    }
    else if (strcasecmp(names[i], "STYLES") == 0) {
      // __TODO__
    }
    else if (strcasecmp(names[i], "SRS") == 0) {
      char **tokens;
      int n;
      // SRS is in format "EPSG:..." or "AUTO:..."
      // For now we support only "EPSG:<epsg_code>" format.
      tokens = split(values[i], ':', &n);
      if (tokens==NULL || n != 2) {
        msSetError(MS_MISCERR, "Wrong number of arguments for SRS.",
                   "msWMSLoadGetMapParams()");
        return msWMSException(map, wmtver);
      }

      if (strcasecmp(tokens[0], "EPSG") == 0) {
        char buffer[20];
        tokens[1][10] = '\0'; // Just in case
        sprintf(buffer, "init=epsg:%s", tokens[1]);

        if (msLoadProjectionString(&(map->projection), buffer) != 0)
          return msWMSException(map, wmtver);
        
        iUnits = GetMapserverUnitUsingProj(&(map->projection));
        if (iUnits != -1)
          map->units = iUnits;
      }
      else {
        msSetError(MS_MISCERR, 
                   "Unsupported SRS namespace (only EPSG currently supported).",
                   "msWMSLoadGetMapParams()");
        return msWMSException(map, wmtver);
      }
      msFreeCharArray(tokens, n);
    }
    else if (strcasecmp(names[i], "BBOX") == 0) {
      char **tokens;
      int n;
      tokens = split(values[i], ',', &n);
      if (tokens==NULL || n != 4) {
        msSetError(MS_MISCERR, "Wrong number of arguments for BBOX.",
                   "msWMSLoadGetMapParams()");
        return msWMSException(map, wmtver);
      }
      map->extent.minx = atof(tokens[0]);
      map->extent.miny = atof(tokens[1]);
      map->extent.maxx = atof(tokens[2]);
      map->extent.maxy = atof(tokens[3]);
      msFreeCharArray(tokens, n);
      adjust_extent = MS_TRUE;
    }
    else if (strcasecmp(names[i], "WIDTH") == 0) {
      map->width = atoi(values[i]);
    }
    else if (strcasecmp(names[i], "HEIGHT") == 0) {
      map->height = atoi(values[i]);
    }
    else if (strcasecmp(names[i], "FORMAT") == 0) {

      format = msSelectOutputFormat( map, values[i] );

      if( format == NULL ) {
        msSetError(MS_IMGERR, 
                   "Unsupported output format (%s).", 
                   "msWMSLoadGetMapParams()",
                   values[i] );
        return msWMSException(map, wmtver);
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
  }

  /*
  ** Apply the selected output format (if one was selected), and override
  ** the transparency if needed.
  */

  if( format != NULL )
      msApplyOutputFormat( &(map->outputformat), format, transparent, 
                           MS_NOOVERRIDE, MS_NOOVERRIDE );

  /*
  ** WMS extents are edge to edge while MapServer extents are center of
  ** pixel to center of pixel.  Here we try to adjust the WMS extents
  ** in by half a pixel.  We wait till here becaus we want to ensure we
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

  return MS_SUCCESS;
}

/*
** printMetadata()
**
** Attempt to output a capability item.  If corresponding metadata is not 
** found then one of a number of predefined actions will be taken. 
** If a default value is provided and metadata is absent then the 
** default will be used.
*/

static int printMetadata(hashTableObj metadata, const char *name, 
                         int action_if_not_found, const char *format, 
                         const char *default_value) 
{
    const char *value;
    int status = MS_NOERR;

    if((value = msLookupHashTable(metadata, (char*)name)))
    { 
        printf(format, value);
    }
    else
    {
        if (action_if_not_found == WMS_WARN)
        {
            printf("<!-- WARNING: Mandatory metadata '%s' was missing in this context. -->\n", name);
            status = action_if_not_found;
        }

        if (default_value)
            printf(format, default_value);
    }

    return status;
}

/*
** printGroupMetadata()
**
** Attempt to output a capability item.  If corresponding metadata is not 
** found then one of a number of predefined actions will be taken. 
** If a default value is provided and metadata is absent then the 
** default will be used.
*/

static int printGroupMetadata(mapObj *map, char* pszGroupName, const char *name, 
                              int action_if_not_found, const char *format, const char *default_value) 
{
    const char *value;
    int status = MS_NOERR;
    int i;

    for (i=0; i<map->numlayers; i++)
    {
       if (map->layers[i].group && (strcmp(map->layers[i].group, pszGroupName) == 0) && map->layers[i].metadata)
       {
         if((value = msLookupHashTable(map->layers[i].metadata, (char*)name)))
         { 
            printf(format, value);
            return status;
         }
       }
    }

    if (action_if_not_found == WMS_WARN)
    {
       printf("<!-- WARNING: Mandatory metadata '%s' was missing in this context. -->\n", name);
       status = action_if_not_found;
    }

    if (default_value)
      printf(format, default_value);
   
    return status;
}

/* printParam()
**
** Same as printMetadata() but applied to mapfile parameters.
**/
static int printParam(const char *name, const char *value, 
                      int action_if_not_found, const char *format, 
                      const char *default_value) 
{
    int status = MS_NOERR;

    if(value && strlen(value) > 0)
    { 
        printf(format, value);
    }
    else
    {
        if (action_if_not_found == WMS_WARN)
        {
            printf("<!-- WARNING: Mandatory mapfile parameter '%s' was missing in this context. -->\n", name);
            status = action_if_not_found;
        }

        if (default_value)
            printf(format, default_value);
    }

    return status;
}

/* printMetadataList()
**
** Prints comma-separated lists metadata.  (e.g. keywordList)
**/
static int printMetadataList(hashTableObj metadata, const char *name, 
                             const char *startTag, const char *endTag,
                             const char *itemFormat) 
{
    const char *value;
    if((value = msLookupHashTable(metadata, (char*)name))) 
    {
      char **keywords;
      int numkeywords;
      
      keywords = split(value, ',', &numkeywords);
      if(keywords && numkeywords > 0) {
        int kw;
	printf("%s", startTag);
	for(kw=0; kw<numkeywords; kw++) 
            printf(itemFormat, keywords[kw]);
	printf("%s", endTag);
	msFreeCharArray(keywords, numkeywords);
      }
      return MS_TRUE;
    }
    return MS_FALSE;
}


/*
**
*/
static void printRequestCap(const char *wmtver, const char *request,
                         const char *script_url, const char *formats, ...)
{
  va_list argp;
  const char *fmt;

  printf("    <%s>\n", request);

  // We expect to receive a NULL-terminated args list of formats
  va_start(argp, formats);
  fmt = formats;
  while(fmt != NULL)
  {
      printf("      <Format>%s</Format>\n", fmt);
      fmt = va_arg(argp, const char *);
  }
  va_end(argp);

  printf("      <DCPType>\n");
  printf("        <HTTP>\n");

  if (strcasecmp(wmtver, "1.0.0") == 0) {
    printf("          <Get onlineResource=\"%s\" />\n", script_url);
    printf("          <Post onlineResource=\"%s\" />\n", script_url);
  } else {
    printf("          <Get><OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:href=\"%s\"/></Get>\n", script_url);
    printf("          <Post><OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:href=\"%s\"/></Post>\n", script_url);
  }

  printf("        </HTTP>\n");
  printf("      </DCPType>\n");
  printf("    </%s>\n", request);
}


/*
**
*/
void msWMSPrintLatLonBoundingBox(const char *tabspace, 
                                 rectObj *extent, projectionObj *srcproj) 
{
  rectObj ll_ext;
  ll_ext = *extent;

  if (srcproj->numargs > 0 && !pj_is_latlong(srcproj->proj)) {
    msProjectRect(srcproj, NULL, &ll_ext);
  }

  printf("%s<LatLonBoundingBox minx=\"%g\" miny=\"%g\" maxx=\"%g\" maxy=\"%g\" />\n", 
         tabspace, ll_ext.minx, ll_ext.miny, ll_ext.maxx, ll_ext.maxy);
}

/*
** Emit a bounding box if we can find projection information.
*/
static void msWMSPrintBoundingBox(const char *tabspace, 
                                  rectObj *extent, 
                                  projectionObj *srcproj,
                                  hashTableObj metadata ) 
{
    const char	*value;

    value = msGetEPSGProj(srcproj, metadata, MS_TRUE);
    
    if( value != NULL )
    {
        printf("%s<BoundingBox SRS=\"%s\"\n"
               "%s            minx=\"%g\" miny=\"%g\" maxx=\"%g\" maxy=\"%g\"",
               tabspace, value, 
               tabspace, extent->minx, extent->miny, 
               extent->maxx, extent->maxy);

        if( msLookupHashTable( metadata, "wms_resx" ) != NULL
            && msLookupHashTable( metadata, "wms_resy" ) != NULL )
            printf( "\n%s            resx=\"%s\" resy=\"%s\"",
                    tabspace, 
                    msLookupHashTable( metadata, "wms_resx" ),
                    msLookupHashTable( metadata, "wms_resy" ) );
        
        printf( " />\n" );
    }
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
extern double inchesPerUnit[];  // defined in mapscale.c
void msWMSPrintScaleHint(const char *tabspace, double minscale, 
                         double maxscale, double resolution) 
{
  double scalehintmin=0.0, scalehintmax=0.0, diag;

  diag = sqrt(2.0);

  if (minscale > 0)
    scalehintmin = diag*(minscale/resolution)/inchesPerUnit[MS_METERS];
  if (maxscale > 0)
    scalehintmax = diag*(maxscale/resolution)/inchesPerUnit[MS_METERS];

  if (scalehintmin > 0.0 || scalehintmax > 0.0)
  {
      printf("%s<ScaleHint min=\"%g\" max=\"%g\" />\n", 
             tabspace, scalehintmin, scalehintmax);
      if (scalehintmax == 0.0)
          printf("%s<!-- WARNING: Only MINSCALE and no MAXSCALE specified in "
                 "the mapfile. A default value of 0 has been returned for the "
                 "Max ScaleHint but this is probably not what you want. -->\n",
                 tabspace);
  }
}


/*
** msWMSGetLayerExtent()
**
** Try to establish layer extent, first looking for "extent" metadata, and
** if not found then open layer to read extent.
**
** __TODO__ Replace metadata with EXTENT param in layerObj???
*/
int msWMSGetLayerExtent(mapObj *map, layerObj *lp, rectObj *ext)
{
  static char *value;

  if ((value = msLookupHashTable(lp->metadata, "wms_extent")) != NULL)
  {
    char **tokens;
    int n;

    tokens = split(value, ' ', &n);
    if (tokens==NULL || n != 4) {
      msSetError(MS_MISCERR, "Wrong number of arguments for EXTENT metadata.",
                 "msWMSGetLayerExtent()");
      return MS_FAILURE;
    }
    ext->minx = atof(tokens[0]);
    ext->miny = atof(tokens[1]);
    ext->maxx = atof(tokens[2]);
    ext->maxy = atof(tokens[3]);

    msFreeCharArray(tokens, n);
    return MS_SUCCESS;
  }
  else if (lp->type == MS_LAYER_RASTER)
  {
    // __TODO__ We need getExtent() for rasters... use metadata for now.
    return MS_FAILURE; 
  }
  else
  {
    if (msLayerOpen(lp, map->shapepath) == MS_SUCCESS) {
      int status;
      status = msLayerGetExtent(lp, ext);
      msLayerClose(lp);
      return status;
    }
  }

  return MS_FAILURE;
}

/*
** msDumpLayer()
*/
int msDumpLayer(mapObj *map, layerObj *lp, const char* wmtver, 
                const char *indent)
{
   rectObj ext;
   const char *value;
   
   if (strcasecmp(wmtver, "1.0.7") <= 0)
   {
       printf("%s    <Layer queryable=\"%d\">\n", 
              indent, msIsLayerQueryable(lp));
   }
   else
   {
       // 1.0.8, 1.1.0 and later: opaque and cascaded are new.
       int cascaded=0, opaque=0;
       if ((value = msLookupHashTable(lp->metadata, "wms_opaque")) != NULL)
           opaque = atoi(value);
       if (lp->connectiontype == MS_WMS)
           cascaded = 1;

       printf("%s    <Layer queryable=\"%d\" opaque=\"%d\" cascaded=\"%d\">\n",
              indent, msIsLayerQueryable(lp), opaque, cascaded);
   }

   printParam("LAYER.NAME", lp->name, WMS_WARN, 
              "        <Name>%s</Name>\n", NULL);
   // the majority of this section is dependent on appropriately named metadata in the LAYER object
   printMetadata(lp->metadata, "wms_title", WMS_WARN,
                 "        <Title>%s</Title>\n", lp->name);

   printMetadata(lp->metadata, "wms_abstract", WMS_NOERR,
                 "        <Abstract>%s</Abstract>\n", NULL);

   printMetadataList(lp->metadata, "wms_keywordlist", 
                     "        <KeywordList>\n", "        </KeywordList>\n",
                     "          <Keyword>%s</Keyword>\n");

   if (msGetEPSGProj(&(map->projection),map->web.metadata,MS_FALSE) == NULL)
   {
       // If map has no proj then every layer MUST have one or produce a warning
       printParam("(at least one of) MAP.PROJECTION, LAYER.PROJECTION or wms_srs metadata", 
                  msGetEPSGProj(&(lp->projection), lp->metadata, MS_FALSE),
                  WMS_WARN, "        <SRS>%s</SRS>\n", NULL);
   }
   else
   {
       // No warning required in this case since there's at least a map proj.
       printParam(" LAYER.PROJECTION (or wms_srs metadata)", 
                  msGetEPSGProj(&(lp->projection), lp->metadata, MS_FALSE),
                  WMS_NOERR, "        <SRS>%s</SRS>\n", NULL);
   }

   // If layer has no proj set then use map->proj for bounding box.
   if (msWMSGetLayerExtent(map, lp, &ext) == MS_SUCCESS)
   {
       if(lp->projection.numargs > 0) 
       {
           msWMSPrintLatLonBoundingBox("        ", &(ext), &(lp->projection));
           msWMSPrintBoundingBox( "        ", &(ext), &(lp->projection), 
                                  lp->metadata );
       } 
       else 
       {
           msWMSPrintLatLonBoundingBox("        ", &(ext), &(map->projection));
           msWMSPrintBoundingBox( "        ", &(ext), &(map->projection), 
                                  map->web.metadata );
       }
   }

   msWMSPrintScaleHint("        ", lp->minscale, lp->maxscale, map->resolution);
    
   printf("%s    </Layer>\n", indent);
      
   return MS_SUCCESS;
}



/*
** msWMSCapabilities()
*/
int msWMSCapabilities(mapObj *map, const char *wmtver) 
{
  const char *value=NULL;
  const char *dtd_url = NULL;
  char *script_url=NULL;
  char *pszMimeType=NULL;

  // Decide which version we're going to return.
  if (wmtver && strcasecmp(wmtver, "1.0.7") < 0) {
    wmtver = "1.0.0";
    dtd_url = "http://www.digitalearth.gov/wmt/xml/capabilities_1_0_0.dtd";
  }
  else if (wmtver && strcasecmp(wmtver, "1.0.8") < 0) {
    wmtver = "1.0.7";
    dtd_url = "http://www.digitalearth.gov/wmt/xml/capabilities_1_0_7.dtd";
  }
  else if (wmtver && strcasecmp(wmtver, "1.1.0") < 0) {
    wmtver = "1.0.8";
    dtd_url = "http://www.digitalearth.gov/wmt/xml/capabilities_1_0_8.dtd";
  }
  else {
    wmtver = "1.1.0";
    dtd_url = "http://www.digitalearth.gov/wmt/xml/capabilities_1_1_0.dtd";
  }

  // We need this script's URL, including hostname.
  // Default to use the value of the "onlineresource" metadata, and if not
  // set then build it: "http://$(SERVER_NAME):$(SERVER_PORT)$(SCRIPT_NAME)?"
  if ((value = msLookupHashTable(map->web.metadata, "wms_onlineresource"))) {
    script_url = strdup(value);
  }
  else {
    const char *hostname, *port, *script, *protocol="http";
    hostname = getenv("SERVER_NAME");
    port = getenv("SERVER_PORT");
    script = getenv("SCRIPT_NAME");

    // HTTPS is set by Apache to "on" in an HTTPS server ... if not set then
    // check SERVER_PORT: 443 is the default https port.
    if ( ((value=getenv("HTTPS")) && strcasecmp(value, "on") == 0) ||
         ((value=getenv("SERVER_PORT")) && atoi(value) == 443) )
    {
        protocol = "https";
    }

    if (hostname && port && script) {
      script_url = (char*)malloc(sizeof(char)*(strlen(hostname)+strlen(port)+strlen(script)+10));
      if (script_url) sprintf(script_url, "%s://%s:%s%s?", protocol, hostname, port, script);
    }
    else {
      msSetError(MS_CGIERR, "Impossible to establish server URL.  Please set \"wms_onlineresource\" metadata.", "msWMSCapabilities()");
      return msWMSException(map, wmtver);
    }
  }
  if (script_url == NULL) {
      msSetError(MS_MEMERR, NULL, "msWMSCapabilities");
      return msWMSException(map, wmtver);
  }

  if (strcasecmp(wmtver, "1.0.7") <= 0) 
      printf("Content-type: text/xml%c%c",10,10);  // 1.0.0 to 1.0.7
  else
      printf("Content-type: application/vnd.ogc.wms_xml%c%c",10,10);  // 1.0.8, 1.1.0 and later

  printMetadata(map->web.metadata, "wms_encoding", WMS_NOERR,
                "<?xml version='1.0' encoding=\"%s\" standalone=\"no\" ?>\n",
                "ISO-8859-1");
  printf("<!DOCTYPE WMT_MS_Capabilities SYSTEM \"%s\"\n", dtd_url);
  printf(" [\n");

  // some mapserver specific declarations will go here
  printf(" <!ELEMENT VendorSpecificCapabilities EMPTY>\n");

  printf(" ]>  <!-- end of DOCTYPE declaration -->\n\n");

  printf("<WMT_MS_Capabilities version=\"%s\" updateSequence=\"0\">\n",wmtver);

  // WMS definition
  printf("<Service> <!-- a service IS a MapServer mapfile -->\n");
  printf("  <Name>GetMap</Name> <!-- WMT defined -->\n");

  // the majority of this section is dependent on appropriately named metadata in the WEB object
  printMetadata(map->web.metadata, "wms_title", WMS_WARN,
                "  <Title>%s</Title>\n", map->name);
  printMetadata(map->web.metadata, "wms_abstract", WMS_NOERR,
                "  <Abstract>%s</Abstract>\n", NULL);
  if (strcasecmp(wmtver, "1.0.0") == 0)
    printf("  <OnlineResource>%s</OnlineResource>\n", script_url);
  else
    printf("  <OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:href=\"%s\"/>\n", script_url);

  printMetadataList(map->web.metadata, "wms_keywordlist", 
                    "  <KeywordList>\n", "  </KeywordList>\n",
                    "    <Keyword>%s</Keyword>\n");
  
  // contact information is a required element in 1.0.7 but the
  // sub-elements such as ContactPersonPrimary, etc. are not!
  // In 1.1.0, ContactInformation becomes optional.
  if (strcasecmp(wmtver, "1.0.0") > 0) 
  {
    printf("  <ContactInformation>\n");
    if(msLookupHashTable(map->web.metadata, "wms_contactperson") ||
       msLookupHashTable(map->web.metadata, "wms_contactorganization")) 
    {
      // ContactPersonPrimary is optional, but when present then all its 
      // sub-elements are mandatory
      printf("    <ContactPersonPrimary>\n");
      printMetadata(map->web.metadata, "wms_contactperson", WMS_WARN,
                    "      <ContactPerson>%s</ContactPerson>\n", NULL);
      printMetadata(map->web.metadata, "wms_contactorganization", WMS_WARN,
                    "      <ContactOrganization>%s</ContactOrganization>\n", NULL);
      printf("    </ContactPersonPrimary>\n");
    }

    printMetadata(map->web.metadata, "wms_contactposition", WMS_NOERR,
                  "    <ContactPosition>%s</ContactPosition>\n", NULL);

    if(msLookupHashTable(map->web.metadata, "wms_addresstype") || 
       msLookupHashTable(map->web.metadata, "wms_address") || 
       msLookupHashTable(map->web.metadata, "wms_city") ||
       msLookupHashTable(map->web.metadata, "wms_stateorprovince") || 
       msLookupHashTable(map->web.metadata, "wms_postcode") ||
       msLookupHashTable(map->web.metadata, "wms_country")) 
    {
      // ContactAdress is optional, but when present then all its 
      // sub-elements are mandatory
      printf("    <ContactAddress>\n");
      printMetadata(map->web.metadata, "wms_addresstype", WMS_WARN,
                    "      <AddressType>%s</AddressType>\n", NULL);
      printMetadata(map->web.metadata, "wms_address", WMS_WARN,
                    "      <Address>%s</Address>\n", NULL);
      printMetadata(map->web.metadata, "wms_city", WMS_WARN,
                    "      <City>%s</City>\n", NULL);
      printMetadata(map->web.metadata, "wms_stateorprovince", WMS_WARN,
                    "      <StateOrProvince>%s</StateOrProvince>\n", NULL);
      printMetadata(map->web.metadata, "wms_postcode", WMS_WARN,
                    "      <PostCode>%s</PostCode>\n", NULL);
      printMetadata(map->web.metadata, "wms_country", WMS_WARN,
                    "      <Country>%s</Country>\n", NULL);
      printf("    </ContactAddress>\n");
    }

    printMetadata(map->web.metadata, "wms_contactvoicetelephone", WMS_NOERR,
                  "    <ContactVoiceTelephone>%s</ContactVoiceTelephone>\n", NULL);
    printMetadata(map->web.metadata, "wms_contactfacsimiletelephone", WMS_NOERR,
                  "    <ContactFacsimileTelephone>%s</ContactFacsimileTelephone>\n", NULL);
    printMetadata(map->web.metadata, "wms_contactelectronicmailaddress", WMS_NOERR,
                  "    <ContactElectronicMailAddress>%s</ContactElectronicMailAddress>\n", NULL);

    printf("  </ContactInformation>\n");
  }

  printMetadata(map->web.metadata, "wms_accessconstraints", WMS_NOERR,
                "  <AccessConstraints>%s</AccessConstraints>\n", NULL);
  printMetadata(map->web.metadata, "wms_fees", WMS_NOERR,
                "  <Fees>%s</Fees>\n", NULL);

  printf("</Service>\n\n");

  // WMS capabilities definitions
  printf("<Capability>\n");
  printf("  <Request>\n");

  if (strcasecmp(wmtver, "1.0.7") <= 0)
  {
    // WMS 1.0.0 to 1.0.7 - We don't try to use outputformats list here for now
    printRequestCap(wmtver, "Map", script_url, 
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
                      , NULL);
    printRequestCap(wmtver, "Capabilities", script_url, "<WMS_XML />", NULL);
    printRequestCap(wmtver, "FeatureInfo", script_url, "<MIME /><GML.1 />", NULL);
  }
  else
  {
    char *mime_list[20];
    // WMS 1.0.8, 1.1.0 and later
    // Note changes to the request names, their ordering, and to the formats

    printRequestCap(wmtver, "GetCapabilities", script_url, 
                    "application/vnd.ogc.wms_xml", 
                    NULL);
    
    msGetOutputFormatMimeList(map,mime_list,sizeof(mime_list)/sizeof(char*));
    printRequestCap(wmtver, "GetMap", script_url,
                    mime_list[0], mime_list[1], mime_list[2], mime_list[3],
                    mime_list[4], mime_list[5], mime_list[6], mime_list[7],
                    mime_list[8], mime_list[9], mime_list[10], mime_list[11],
                    mime_list[12], mime_list[13], mime_list[14], mime_list[15],
                    mime_list[16], mime_list[17], mime_list[18], mime_list[19],
                    NULL );

    pszMimeType = msLookupHashTable(map->web.metadata, "WMS_FEATURE_INFO_MIME_TYPE");
    if (pszMimeType && strcasecmp(pszMimeType, "NONE") == 0)
       printRequestCap(wmtver, "GetFeatureInfo", script_url,
                       "text/plain",
                       "application/vnd.ogc.gml",
                       NULL);
    else
    if (pszMimeType)
       printRequestCap(wmtver, "GetFeatureInfo", script_url,
                       "text/plain",
                       pszMimeType,
                       "application/vnd.ogc.gml",
                       NULL);
    else
       printRequestCap(wmtver, "GetFeatureInfo", script_url,
                       "text/plain",
                       "text/html",
                       "application/vnd.ogc.gml",
                       NULL);       
       
       
  }

  printf("  </Request>\n");

  printf("  <Exception>\n");
  if (strcasecmp(wmtver, "1.0.7") <= 0) 
      printf("    <Format><BLANK /><INIMAGE /><WMS_XML /></Format>\n");
  else
  {
      // 1.0.8, 1.1.0 and later
      printf("    <Format>application/vnd.ogc.se_xml</Format>\n");
      printf("    <Format>application/vnd.ogc.se_inimage</Format>\n");
      printf("    <Format>application/vnd.ogc.se_blank</Format>\n");
  }
  printf("  </Exception>\n");


  printf("  <VendorSpecificCapabilities />\n"); // nothing yet


  // Top-level layer with map extents and SRS, encloses all map layers
  printf("  <Layer>\n");

  // Layer Name is optional but title is mandatory.
  printParam("MAP.NAME", map->name, WMS_NOERR, "    <Name>%s</Name>\n", NULL);
  printMetadata(map->web.metadata, "wms_title", WMS_WARN,
                "    <Title>%s</Title>\n", map->name);

  // According to normative comments in the 1.0.7 DTD, the root layer's SRS tag
  // is REQUIRED.  It also suggests that we use an empty SRS element if there
  // is no common SRS.
  printParam("MAP.PROJECTION (or wms_srs metadata)",
             msGetEPSGProj(&(map->projection), map->web.metadata, MS_FALSE),
             WMS_WARN, "    <SRS>%s</SRS>\n", "");

  msWMSPrintLatLonBoundingBox("    ", &(map->extent), &(map->projection));
  msWMSPrintBoundingBox( "    ", &(map->extent), &(map->projection),
                         map->web.metadata );
  msWMSPrintScaleHint("    ", map->web.minscale, map->web.maxscale,
                      map->resolution);


  //
  // Dump list of layers organized by groups.  Layers with no group are listed
  // individually, at the same level as the groups in the layer hierarchy
  //
  if (map->numlayers)
  {
     int i, j;
     char *pabLayerProcessed = NULL;

     // We'll use this array of booleans to track which layer/group have been
     // processed already
     pabLayerProcessed = (char *)calloc(map->numlayers, sizeof(char*));
   
     for(i=0; i<map->numlayers; i++)
     {
         layerObj *lp;
         lp = &(map->layers[i]);

         if (pabLayerProcessed[i])
             continue;  // Layer has already been handled

         if (lp->group == NULL || strlen(lp->group) == 0)
         {
             // This layer is not part of a group... dump it directly
             msDumpLayer(map, lp, wmtver, "");
             pabLayerProcessed[i] = 1;
         }
         else
         {
             // Beginning of a new group... enclose the group in a layer block
             printf("    <Layer>\n");

             // Layer Name is optional but title is mandatory.
             printParam("GROUP.NAME", lp->group, WMS_NOERR, 
                        "      <Name>%s</Name>\n", NULL);
             printGroupMetadata(map, lp->group, "WMS_GROUP_TITLE", WMS_WARN, 
                                "      <Title>%s</Title>\n", lp->group);

             // Dump all layers for this group
             for(j=i; j<map->numlayers; j++)
             {
                 if (!pabLayerProcessed[j] &&
                     map->layers[j].group && 
                     strcmp(lp->group, map->layers[j].group) == 0 )
                 {
                     msDumpLayer(map, &(map->layers[j]), wmtver, "  ");
                     pabLayerProcessed[j] = 1;
                 }
             }

             // Close group layer block
             printf("    </Layer>\n");
         }
     }

     free(pabLayerProcessed);
  } 
   
  printf("  </Layer>\n");

  printf("</Capability>\n");
  printf("</WMT_MS_Capabilities>\n");

  free(script_url);

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
         
         layers = split(values[i], ',', &tok);
         
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
         
         layers = split(values[i], ',', &tok);

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
         
         imgext = gsub(values[i], ",", " ");
         
         values[tmpNumentries] = strdup(imgext);
         names[tmpNumentries] = strdup("imgext");
            
         tmpNumentries++;
         
         free(imgext);
      }      
   }
   
   *numentries = tmpNumentries;
   
   return MS_SUCCESS;
}

/*
** msWMSGetMap()
*/
int msWMSGetMap(mapObj *map, const char *wmtver) 
{
    imageObj *img;

  // __TODO__ msDrawMap() will try to adjust the extent of the map
  // to match the width/height image ratio.
  // The spec states that this should not happen so that we can deliver
  // maps to devices with non-square pixels.

  img = msDrawMap(map);
  if (img == NULL)
      return msWMSException(map, wmtver);

  printf("Content-type: %s%c%c", MS_IMAGE_MIME_TYPE(map->outputformat), 10,10);
  if (msSaveImage(map, img, NULL) != MS_SUCCESS)
      return msWMSException(map, wmtver);

  msFreeImage(img);

  return(MS_SUCCESS);
}

int msDumpResult(mapObj *map, int bFormatHtml, const char* wmtver, int feature_count)
{
   int numresults=0;
   int i;
   
   for(i=0; i<map->numlayers && numresults<feature_count; i++) 
   {
      int j, k;
      layerObj *lp;
      lp = &(map->layers[i]);

      if(lp->status != MS_ON || lp->resultcache==NULL || lp->resultcache->numresults == 0)
        continue;

      if(msLayerOpen(lp, map->shapepath) != MS_SUCCESS || msLayerGetItems(lp) != MS_SUCCESS)
        return msWMSException(map, wmtver);

      printf("\nLayer '%s'\n", lp->name);

      for(j=0; j<lp->resultcache->numresults && numresults<feature_count; j++) {
        shapeObj shape;

        msInitShape(&shape);
        if (msLayerGetShape(lp, &shape, lp->resultcache->results[j].tileindex, lp->resultcache->results[j].shapeindex) != MS_SUCCESS)
          return msWMSException(map, wmtver);

        printf("  Feature %ld: \n", lp->resultcache->results[j].shapeindex);
        
        for(k=0; k<lp->numitems; k++)
	  printf("    %s = '%s'\n", lp->items[k], shape.values[k]);

        msFreeShape(&shape);
        numresults++;
      }

      msLayerClose(lp);
    }
   
   return numresults;
}


/*
** msWMSFeatureInfo()
*/
int msWMSFeatureInfo(mapObj *map, const char *wmtver, char **names, char **values, int numentries)
{
  int i, feature_count=1, numlayers_found=0;
  pointObj point = {-1.0, -1.0};
  const char *info_format="MIME";
  double cellx, celly;
  errorObj *ms_error = msGetErrorObj();
  int status;
  const char *pszMimeType=NULL;
   
  pszMimeType = msLookupHashTable(map->web.metadata, "WMS_FEATURE_INFO_MIME_TYPE");
  if (!pszMimeType)
  {
     pszMimeType = "text/html";
  }
   
   
  for(i=0; map && i<numentries; i++) {
    if(strcasecmp(names[i], "QUERY_LAYERS") == 0) {
      char **layers;
      int numlayers, j, k;

      layers = split(values[i], ',', &numlayers);
      if(layers==NULL || numlayers < 1) {
        msSetError(MS_MISCERR, "At least one layer name required in QUERY_LAYERS.", "msWMSFeatureInfo()");
        return msWMSException(map, wmtver);
      }

      for(j=0; j<map->numlayers; j++) {
        // Force all layers OFF by default
	map->layers[j].status = MS_OFF;

        for(k=0; k<numlayers; k++) {
	  if(strcasecmp(map->layers[j].name, layers[k]) == 0) {
	    map->layers[j].status = MS_ON;
            numlayers_found++;
          }
        }
      }

      msFreeCharArray(layers, numlayers);
    } else if (strcasecmp(names[i], "INFO_FORMAT") == 0)
      info_format = values[i];
    else if (strcasecmp(names[i], "FEATURE_COUNT") == 0) 
      feature_count = atoi(values[i]);
    else if(strcasecmp(names[i], "X") == 0) 
      point.x = atof(values[i]);
    else if (strcasecmp(names[i], "Y") == 0)
      point.y = atof(values[i]);
  }

  if(numlayers_found == 0) {
    msSetError(MS_MISCERR, "Required QUERY_LAYERS parameter missing for getFeatureInfo.", "msWMSFeatureInfo()");
    return msWMSException(map, wmtver);
  }

  if(point.x == -1.0 || point.y == -1.0) {
    msSetError(MS_MISCERR, "Required X/Y parameters missing for getFeatureInfo.", "msWMSFeatureInfo()");
    return msWMSException(map, wmtver);
  }

  // Perform the actual query
  cellx = MS_CELLSIZE(map->extent.minx, map->extent.maxx, map->width); // note: don't adjust extent, WMS assumes incoming extent is correct
  celly = MS_CELLSIZE(map->extent.miny, map->extent.maxy, map->height);
  point.x = MS_IMAGE2MAP_X(point.x, map->extent.minx, cellx);
  point.y = MS_IMAGE2MAP_Y(point.y, map->extent.maxy, celly);

  if(msQueryByPoint(map, -1, (feature_count==1?MS_SINGLE:MS_MULTIPLE), point, 0) != MS_SUCCESS)
    if(ms_error->code != MS_NOTFOUND) return msWMSException(map, wmtver);

  // Generate response
  if (strcasecmp(info_format, "MIME") == 0 ||
      strcasecmp(info_format, "text/plain") == 0) {

    // MIME response... we're free to use any valid MIME type
    int numresults = 0;

    printf("Content-type: text/plain%c%c", 10,10);
    printf("GetFeatureInfo results:\n");

    numresults = msDumpResult(map, 0, wmtver, feature_count);
     
    if (numresults == 0) printf("\n  Search returned no results.\n");

  } else if (strncasecmp(info_format, "GML", 3) == 0 ||  // accept GML.1 or GML
             strcasecmp(info_format, "application/vnd.ogc.gml") == 0) {

    if (strcasecmp(wmtver, "1.0.7") <= 0)
        printf("Content-type: text/xml%c%c", 10,10);
    else
        printf("Content-type: application/vnd.ogc.gml%c%c", 10,10);
    
    msGMLWriteQuery(map, NULL); // default is stdout

  } else
  if (pszMimeType && (strcmp(pszMimeType, info_format) == 0))
  {
     mapservObj *msObj;

     msObj = msAllocMapServObj();

     // Translate some vars from WMS to mapserv
     msTranslateWMS2Mapserv(names, values, &numentries);

     msObj->Map = map;
     msObj->ParamNames = names;
     msObj->ParamValues = values;
     msObj->Mode = QUERY;
     sprintf(msObj->Id, "%ld%d",(long)time(NULL),(int)getpid()); // asign now so it can be overridden
     msObj->NumParams = numentries;
     msObj->MapPnt.x = point.x;
     msObj->MapPnt.y = point.y;
     
     if ((status = msReturnTemplateQuery(msObj, (char*)pszMimeType)) != MS_SUCCESS)
         return status;

     msFreeMapServObj(msObj);
  }
  else 
  {
     msSetError(MS_MISCERR, "Unsupported INFO_FORMAT value (%s).", "msWMSFeatureInfo()", info_format);
     return msWMSException(map, wmtver);
  }

  return(MS_SUCCESS);
}

#endif /* USE_WMS */


/*
** msWMSDispatch() is the entry point for WMS requests.
** - If this is a valid request then it is processed and MS_SUCCESS is returned
**   on success, or MS_FAILURE on failure.
** - If this does not appear to be a valid WMS request then MS_DONE
**   is returned and MapServer is expected to process this as a regular
**   MapServer request.
*/
int msWMSDispatch(mapObj *map, char **names, char **values, int numentries)
{
#ifdef USE_WMS
  int i, status;
  static char *wmtver = NULL, *request=NULL, *service=NULL;

  /*
  ** Process Params common to all requests
  */
  // VERSION (WMTVER in 1.0.0) and REQUEST must be present in a valid request
  for(i=0; i<numentries; i++) {
    if (strcasecmp(names[i], "WMTVER") == 0 || strcasecmp(names[i], "VERSION") == 0)
      wmtver = values[i];
    else if (strcasecmp(names[i], "REQUEST") == 0)
      request = values[i];
    else if (strcasecmp(names[i], "EXCEPTIONS") == 0)
      wms_exception_format = values[i];
    else if (strcasecmp(names[i], "SERVICE") == 0)
      service = values[i];
  }

  /*
  ** Dispatch request... we should probably do some validation on VERSION here
  ** vs the versions we actually support.
  */
  if ((service == NULL || strcasecmp(service, "WMS") == 0) &&
      request && (strcasecmp(request, "capabilities") == 0 ||
                  strcasecmp(request, "GetCapabilities") == 0) ) 
  {
      if (!wmtver)
          wmtver = "1.1.0";  // VERSION is optional with getCapabilities only
      if ((status = msWMSMakeAllLayersUnique(map, wmtver)) != MS_SUCCESS)
          return status;
      return msWMSCapabilities(map, wmtver);
  }

  // VERSION *and* REQUEST required by both getMap and getFeatureInfo
  if (wmtver==NULL || request==NULL) return MS_DONE; // Not a WMS request

  if ((status = msWMSMakeAllLayersUnique(map, wmtver)) != MS_SUCCESS)
      return status;

  // getMap parameters are used by both getMap and getFeatureInfo
  status = msWMSLoadGetMapParams(map, wmtver, names, values, numentries);
  if (status != MS_SUCCESS) return status;

  if (strcasecmp(request, "map") == 0 || strcasecmp(request, "GetMap") == 0)
    return msWMSGetMap(map, wmtver);
  else if (strcasecmp(request, "feature_info") == 0 || strcasecmp(request, "GetFeatureInfo") == 0)
    return msWMSFeatureInfo(map, wmtver, names, values, numentries);

  // Hummmm... incomplete or unsupported WMS request
  msSetError(MS_MISCERR, "Incomplete or unsupported WMS request", "msWMSDispatch()");
  return msWMSException(map, wmtver);
#else
  msSetError(MS_MISCERR, "WMS support is not available.", "msWMSDispatch()");
  return(MS_FAILURE);
#endif
}


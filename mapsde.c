#include "map.h"
#include "maperror.h"

/* ESRI SDE Client Includes */
#ifdef USE_SDE
#include <sdetype.h>
#include <sdeerno.h>
#endif

#ifdef USE_SDE
static void set_error(long error_code, char *routine, char *sde_routine) {
  char error_string[SE_MAX_MESSAGE_LENGTH];

  error_string[0] = '\0';
  SE_error_get_string(error_code, error_string);

  msSetError(MS_SDEERR, NULL, routine);
  sprintf(ms_error.message, "%s: %s", sde_routine, error_string);

  return;
}

static int sde_2_internal(SE_SHAPE *sdeshape, shapeObj *shape) {
  return(0);
}
#endif

int msDrawSDELayer(mapObj *map, layerObj *layer, gdImagePtr img) {
#ifdef USE_SDE
  int i,j;

  SE_CONNECTION connection=0;
  SE_STREAM stream=0;
  SE_ERROR error;
  long status;

  SE_COORDREF coordref=NULL;
  SE_LAYERINFO layerinfo;

  SE_ENVELOPE rect;

  SE_SQL_CONSTRUCT *sql;

  char *annotation;
  
  char **params;
  int numparams;

  if((layer->status != MS_ON) && (layer->status != MS_DEFAULT))
    return(0);

  params = split(layer->connection, ',', &numparams);
  if(!params) {
    msSetError(MS_MEMERR, "Error spliting SDE connection information.", "msDrawSDELayer()");
    return(-1);
  }

  if(numparams < 5) {
    msSetError(MS_SDEERR, "Not enough SDE connection parameters specified.", "msDrawSDELayer()");
    return(-1);
  }

  status = SE_connection_create(params[0], params[1], params[2], params[3], params[4], &error, &connection);
  if(status != SE_SUCCESS) {
    set_error(status, "msDrawSDELayer()", "SE_connection_create()");
    return(-1);
  }

  msFreeCharArray(params, numparams);

  status = SE_stream_create(connection, &stream);
  if(status != SE_SUCCESS) {
    set_error(status, "msDrawSDELayer()", "SE_stream_create()");
    return(-1);
  }

  /*
  ** Get some basic information about the layer
  */
  SE_coordref_create(&coordref);
  SE_layerinfo_create(coordref, &layerinfo);

  params = split(layer->data, ',', &numparams);
  if(!params) {
    msSetError(MS_MEMERR, "Error spliting SDE layer information.", "msDrawSDELayer()");
    return(-1);
  }

  if(numparams < 2) {
    msSetError(MS_SDEERR, "Not enough SDE layer parameters specified.", "msDrawSDELayer()");
    return(-1);
  }

  status = SE_layer_get_info(connection, params[0], params[1], layerinfo);
  if(status != SE_SUCCESS) {
    set_error(status, "msDrawSDELayer()", "SE_layer_get_info()");
    return(-1);
  }

  msFreeCharArray(params, numparams);

  status = SE_layerinfo_get_envelope(layerinfo, &rect);
  if(status != SE_SUCCESS) {
    set_error(status, "msDrawSDELayer()", "SE_layerinfo_get_envelope()");
    return(-1);
  }
  
  if(rect.minx > map->extent.maxx) return(0); // msRectOverlap()
  if(rect.maxx < map->extent.minx) return(0);
  if(rect.miny > map->extent.maxy) return(0);
  if(rect.maxy < map->extent.miny) return(0);

  /*
  ** each class is a SQL statement, no expression means all features
  */
  for(i=0; i<layer->numclasses; i++) {

    // step 1: build the query

  }

  SE_stream_free(stream);
  SE_connection_free(connection);
  return(0);
#else
  msSetError(MS_MISCERR, "SDE support is not available.", "msDrawSDELayer()");
  return(-1);
#endif
}

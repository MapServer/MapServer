#include "map.h"
#include "maperror.h"

/* ESRI SDE Client Includes */
#ifdef USE_SDE
#include <sdetype.h>
#include <sdeerno.h>
#endif

#ifdef USE_SDE
static void set_error(long error_code, char *routine, char *sde_routine) {
  char erron_string[SE_MAX_MESSAGE_LENGTH];

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
  SE_CONNECTION connection;
  SE_STREAM stream;
  SE_ERROR error;
  long status;

  SE_SQL_CONSTRUCT *sql;

  char *annotation;
  
  char **params;
  int numparams;

  params = split(layer->connection, ',', &numparams);
  if(!params) {
    msSetError(MS_MEMERR, "Error spliting SDE connection information.", "msDrawSDELayer()");
    return(-1);
  }

  if(numparams < 6) {
    msSetError(MS_SDEERR, "Not enough SDE connection parameters specified.", "msDrawSDELayer()");
    return(-1);
  }

  status = SE_connection_create(params[1], params[2], params[3], params[4], params[5], &error, &connection);
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

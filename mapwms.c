#include "map.h"
#include "maperror.h"

/* 
** Future home of the MapServer OpenGIS WMS module.
*/

/*
** msWMSDispatch() is the entry point for WMS requests.
** - If this is a valid request then it is processed and MS_SUCCESS is returned
**   on success, or MS_FAILURE on failure.
** - If this does not appear to be a valid WMS request then MS_DONE
**   is returned and MapServer is expected to process this as a regular
**   MapServer request.
*/
int msWMSDispatch(mapObj *map, char **names, char **values, int numentries) {
  int i;
  static char *wmtver = NULL, *request=NULL;

  /*
  ** Process Form parameters
  */
  for(i=0; i<numentries; i++)
  {
      // WMTVER and REQUEST must be present in a valid request
      if (strcasecmp(names[i], "WMTVER") == 0) {
          wmtver = values[i];
      }
      else if (strcasecmp(names[i], "REQUEST") == 0) {
          request = values[i];
      }

      // All the rest are optional
      // __TODO__
  }

  /*
  ** Dispatch request... we should probably do some validation on WMTVER here
  ** vs the versions we actually support.
  */
  if (wmtver && strcasecmp(request, "capabilities") == 0) {
      return msWMSCapabilities(map);
  }
  else if (wmtver && strcasecmp(request, "map") == 0) {
      return msWMSGetMap(map);
  }
  else if (wmtver && strcasecmp(request, "featureinfo") == 0) {
      return msWMSFeatureInfo(map);
  }

  // This was not a WMS request... return MS_DONE
  return(MS_DONE);
}

/*
** msWMSCapabilities()
*/
int msWMSCapabilities(mapObj *map) {

  printf("Content-type: application/xml%c%c",10,10);

  if(!map) {
    // return a default capabilties document
    return(MS_SUCCESS);
  }

  // use the given map object to construct a capabilities document

  return(MS_SUCCESS);
}

/*
** msWMSGetMap()
*/
int msWMSGetMap(mapObj *map) {
  printf("Content-type: text/plain\n\n");
  printf("getMap request received... not implemented yet.\n");

  return(MS_SUCCESS);
}

/*
** msWMSFeatureInfo()
*/
int msWMSFeatureInfo(mapObj *map) {
  printf("Content-type: text/plain\n\n");
  printf("getFeatureInfo request received... not implemented yet.\n");

  return(MS_SUCCESS);
}

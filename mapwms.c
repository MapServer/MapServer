#include "map.h"
#include "maperror.h"

/* 
** Future home of the MapServer OpenGIS WMS module.
*/
int msWMSProcessForm(mapObj *map, char **entries, int numentries) {
  return(MS_SUCCESS);
}

int msWMSCapabilities(mapObj *map) {

  printf("Content-type: application/xml%c%c",10,10);

  if(!map) {
    // return a default capabilties document
    return(MS_SUCCESS);
  }

  // use the given map object to construct a capabilities document

  return(MS_SUCCESS);
}

int msWMSGetMap(mapObj *map) {
  return(MS_SUCCESS);
}

int msWMSFeatureInfo(mapObj *map) {
  return(MS_SUCCESS);
}

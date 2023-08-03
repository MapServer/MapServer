#include "mapserver.h"


mapObj* umnms_new_map(char *filename) {
  mapObj *map = NULL;
  if(filename) {  
    map = msLoadMap(filename,NULL);
  } else {
    map = (mapObj *)msSmallCalloc(sizeof(mapObj),1);
    if(initMap(map) == -1) {
      free(map);
      return NULL;
    }
  }
  return map;
}
layerObj* umnms_new_layer(mapObj *map);
classObj* umnms_new_class(layerObj *layer);
styleObj* umnms_new_style(classObj *theclass);
labelObj* umnms_new_label(classObj *theclass);

#ifndef MAPSDE_H
#define MAPSDE_H

#ifdef __cplusplus
extern "C" {
#endif

/* ESRI SDE Client Includes */
#ifdef USE_SDE
#include <sdetype.h>
#include <sdeerno.h>

struct sdeLayerObj { 
  SE_CONNECTION connection;
  SE_LAYERINFO layerinfo;
  SE_COORDREF coordref;  
  SE_STREAM stream;
};

#endif

#ifdef __cplusplus
}
#endif

#endif /* MAP_H */
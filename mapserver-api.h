/* 
 * File:   mapserver-api.h
 * Author: tbonfort
 *
 * Created on March 14, 2013, 1:12 PM
 */

#ifndef MAPSERVER_API_H
#define	MAPSERVER_API_H

#include "mapserver-version.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct mapObj mapObj;
typedef struct layerObj layerObj;
typedef struct classObj classObj;
typedef struct styleObj styleObj;
typedef struct labelObj labelObj;
typedef struct symbolObj symbolObj;
typedef struct imageObj imageObj;

mapObj* umnms_new_map(char *filename);
layerObj* umnms_new_layer(mapObj *map);
classObj* umnms_new_class(layerObj *layer);
styleObj* umnms_new_style(classObj *theclass);
labelObj* umnms_new_label(classObj *theclass);




#ifdef	__cplusplus
}
#endif

#endif	/* MAPSERVER_API_H */


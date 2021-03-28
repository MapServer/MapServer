#ifndef MAPSERVER_CONTEXT_H
#define MAPSERVER_CONTEXT_H

#include "maphash.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MS_CONTEXT_PATH "" // set at compile time
#define MS_CONTEXT_FILENAME "mapserver.ctx"

enum MS_CONTEXT_SECTIONS { MS_CONTEXT_SECTION=3000, MS_CONTEXT_SECTION_ENVIRONMENT, MS_CONTEXT_SECTION_MAPS, MS_CONTEXT_SECTION_PLUGINS };

#ifndef SWIG
typedef struct {
  hashTableObj environment;
  hashTableObj maps;
  hashTableObj plugins;
} contextObj;
#endif /*SWIG*/

MS_DLL_EXPORT contextObj *msLoadContext();
MS_DLL_EXPORT void msFreeContext(contextObj *context);

#ifdef __cplusplus
} /* extern C */
#endif

#endif 

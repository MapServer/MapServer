#ifndef MAPSERVER_CONTEXT_H
#define MAPSERVER_CONTEXT_H

#include "maphash.h"

#ifdef __cplusplus
extern "C" {
#endif

enum MS_CONTEXT_SECTIONS { MS_CONTEXT_SECTION=3000, MS_CONTEXT_SECTION_ENV, MS_CONTEXT_SECTION_MAPS, MS_CONTEXT_SECTION_PLUGINS };

#ifndef SWIG
typedef struct {
  hashTableObj env;
  hashTableObj maps;
  hashTableObj plugins;
} contextObj;
#endif /*SWIG*/

MS_DLL_EXPORT contextObj *msLoadContext();
MS_DLL_EXPORT void msFreeContext(contextObj *context);
MS_DLL_EXPORT const char *msContextGetEnv(contextObj *context, const char *key);
MS_DLL_EXPORT const char *msContextGetMap(contextObj *context, const char *key);

#ifdef __cplusplus
} /* extern C */
#endif

#endif 

#ifndef MAPSERV_CONFIG_H
#define MAPSERV_CONFIG_H

#include "maphash.h"

#ifdef __cplusplus
extern "C" {
#endif

enum MS_CONFIG_SECTIONS { MS_CONFIG_SECTION=3000, MS_CONFIG_SECTION_ENV, MS_CONFIG_SECTION_MAPS, MS_CONFIG_SECTION_PLUGINS };

#ifndef SWIG
typedef struct {
  hashTableObj env;
  hashTableObj maps;
  hashTableObj plugins;
} configObj;
#endif /*SWIG*/

MS_DLL_EXPORT configObj *msLoadConfig(char* ms_config_file);
MS_DLL_EXPORT void msFreeConfig(configObj *config);
MS_DLL_EXPORT const char *msConfigGetEnv(configObj *config, const char *key);
MS_DLL_EXPORT const char *msConfigGetMap(configObj *config, const char *key);

#ifdef __cplusplus
} /* extern C */
#endif

#endif 

#ifndef MAPSERV_CONFIG_H
#define MAPSERV_CONFIG_H

#include "maphash.h"

#ifdef __cplusplus
extern "C" {
#endif

enum MS_CONFIG_SECTIONS { MS_CONFIG_SECTION=3000, MS_CONFIG_SECTION_ENV, MS_CONFIG_SECTION_MAPS, MS_CONFIG_SECTION_PLUGINS };

/**
The :ref:`CONFIG <config>` object
*/
typedef struct {
  hashTableObj env; ///< Key-value pairs of environment variables and values
  hashTableObj maps; ///< Key-value pairs of Mapfile names and paths
  hashTableObj plugins; ///< Key-value pairs of plugin names and paths
} configObj;

MS_DLL_EXPORT configObj *msLoadConfig(const char* ms_config_file);
MS_DLL_EXPORT void msFreeConfig(configObj *config);
MS_DLL_EXPORT const char *msConfigGetEnv(const configObj *config, const char *key);
MS_DLL_EXPORT const char *msConfigGetMap(const configObj *config, const char *key);

#ifdef __cplusplus
} /* extern C */
#endif

#endif 

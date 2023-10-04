/**********************************************************************
 * $id$
 *
 * Project:  MapServer
 * Purpose:  Config File Support
 * Author:   Steve Lime and the MapServer team.
 *
 **********************************************************************
 * Copyright (c) 1996-2005 Regents of the University of Minnesota.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 ****************************************************************************/
#include "mapserver.h"
#include "mapfile.h" // format shares a couple of tokens

#include "cpl_conv.h"

#include <string>

extern "C" int msyylex(void);
extern "C" void msyyrestart(FILE *);

extern "C" FILE *msyyin;
extern "C" int msyystate;
extern "C" int msyylineno;
extern "C" char *msyystring_buffer;

static int initConfig(configObj *config) {
  if (config == NULL) {
    msSetError(MS_MEMERR, "Config object is NULL.", "initConfig()");
    return MS_FAILURE;
  }

  if (initHashTable(&(config->env)) != MS_SUCCESS)
    return MS_FAILURE;
  if (initHashTable(&(config->maps)) != MS_SUCCESS)
    return MS_FAILURE;
  if (initHashTable(&(config->plugins)) != MS_SUCCESS)
    return MS_FAILURE;

  return MS_SUCCESS;
}

void msFreeConfig(configObj *config) {
  if (config == NULL)
    return;
  msFreeHashItems(&(config->env));
  msFreeHashItems(&(config->maps));
  msFreeHashItems(&(config->plugins));

  msFree(config);
}

static int loadConfig(configObj *config) {
  int token;

  if (config == NULL)
    return MS_FAILURE;

  token = msyylex();
  if (token != MS_CONFIG_SECTION) {
    msSetError(MS_IDENTERR,
               "First token must be CONFIG, this doesn't look like a mapserver "
               "config file.",
               "msLoadConfig()");
    return MS_FAILURE;
  }

  for (;;) {
    switch (msyylex()) {
    case (MS_CONFIG_SECTION_ENV): {
      if (loadHashTable(&(config->env)) != MS_SUCCESS)
        return MS_FAILURE;
      break;
    }
    case (MS_CONFIG_SECTION_MAPS):
      if (loadHashTable(&config->maps) != MS_SUCCESS)
        return MS_FAILURE;
      break;
    case (MS_CONFIG_SECTION_PLUGINS):
      if (loadHashTable(&config->plugins) != MS_SUCCESS)
        return MS_FAILURE;
      break;
    case (EOF):
      msSetError(MS_EOFERR, NULL, "msLoadConfig()");
      return MS_FAILURE;
    case (END):
      if (msyyin) {
        fclose(msyyin);
        msyyin = NULL;
      }
      return MS_SUCCESS;
      break;
    default:
      msSetError(MS_IDENTERR, "Parsing error near (%s):(line %d)",
                 "msLoadConfig()", msyystring_buffer, msyylineno);
      return MS_FAILURE;
    }
  }
}

static void msConfigSetConfigOption(const char *key, const char *value) {
  CPLSetConfigOption(key, value);
  if (strcasecmp(key, "PROJ_DATA") == 0 || strcasecmp(key, "PROJ_LIB") == 0) {
    msSetPROJ_DATA(value, nullptr);
  }
}

configObj *msLoadConfig(const char *ms_config_file) {
  configObj *config = NULL;

  if (ms_config_file == NULL) {
    // get config filename from environment
    ms_config_file = getenv("MAPSERVER_CONFIG_FILE");
  }

  if (ms_config_file == NULL && MAPSERVER_CONFIG_FILE[0] != '\0') {
    // Fallback to hardcoded file name
    ms_config_file = MAPSERVER_CONFIG_FILE;
  }

  if (ms_config_file == NULL) {
    msSetError(MS_MISCERR, "No config file set.", "msLoadConfig()");
    return NULL;
  }

  config = (configObj *)calloc(sizeof(configObj), 1);
  MS_CHECK_ALLOC(config, sizeof(configObj), NULL);

  if (initConfig(config) != MS_SUCCESS) {
    msFreeConfig(config);
    return NULL;
  }

  msAcquireLock(TLOCK_PARSER);

  if ((msyyin = fopen(ms_config_file, "r")) == NULL) {
    msDebug("Cannot open configuration file %s.\n", ms_config_file);
    msSetError(MS_IOERR,
               "See mapserver.org/mapfile/config.html for more information.",
               "msLoadConfig()");
    msReleaseLock(TLOCK_PARSER);
    msFreeConfig(config);
    return NULL;
  }

  msyystate = MS_TOKENIZE_CONFIG;
  msyylex(); // sets things up, but doesn't process any tokens

  msyyrestart(msyyin); // start at line 1
  msyylineno = 1;

  if (loadConfig(config) != MS_SUCCESS) {
    msFreeConfig(config);
    msReleaseLock(TLOCK_PARSER);
    if (msyyin) {
      fclose(msyyin);
      msyyin = NULL;
    }
    return NULL;
  }
  msReleaseLock(TLOCK_PARSER);

  // load all env key/values using CPLSetConfigOption() - only do this *after*
  // we have a good config
  const char *key = msFirstKeyFromHashTable(&config->env);
  if (key != NULL) {
    msConfigSetConfigOption(key, msLookupHashTable(&config->env, key));

    const char *last_key = key;
    while ((key = msNextKeyFromHashTable(&config->env, last_key)) != NULL) {
      msConfigSetConfigOption(key, msLookupHashTable(&config->env, key));
      last_key = key;
    }
  }

  return config;
}

const char *msConfigGetEnv(const configObj *config, const char *key) {
  if (config == NULL || key == NULL)
    return NULL;
  return msLookupHashTable(&config->env, key);
}

const char *msConfigGetMap(const configObj *config, const char *key) {
  if (config == NULL || key == NULL)
    return NULL;
  return msLookupHashTable(&config->maps, key);
}

const char *msConfigGetPlugin(const configObj *config, const char *key) {
  if (config == NULL || key == NULL)
    return NULL;
  return msLookupHashTable(&config->plugins, key);
}

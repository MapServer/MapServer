/*
 *  mod_mapserver.c -- Apache sample wms module
 */

#include "httpd.h"
#include "http_config.h"
#include "http_protocol.h"
#include "http_log.h"
#include "ap_config.h"
#include "apr.h"
#include "apr_strings.h"
#include "apr_tables.h"
#include "apr_file_info.h"

#include "../mapserver.h" /* for mapObj */
#include "../cgiutil.h"
#include "../apps/mapserv.h"

module AP_MODULE_DECLARE_DATA mapserver_module;

typedef struct {
  apr_pool_t *config_pool;
  apr_time_t mtime;
  char *mapfile_name;
  char *uri;
  mapObj *map;
} mapserver_dir_config;

/* These are the IO redirection hooks. They are mostly copied over from
 * mapio.c. Note that cbData contains Apache's request_rec!
 */
static int msIO_apacheWrite(void *cbData, void *data, int byteCount) {
  /* simply use the block writing function which is very similar to fwrite */
  return ap_rwrite(data, byteCount, (request_rec *)cbData);
}

static int msIO_apacheError(void *cbData, void *data, int byteCount) {
  /* error reporting is done through the log file... */
  ap_log_error(APLOG_MARK, APLOG_ERR, 0, NULL, "%s", (char *)data);
  return strlen((char *)data);
}

int msIO_installApacheRedirect(request_rec *r) {
  msIOContext stdout_ctx, stderr_ctx;

  stdout_ctx.label = "apache";
  stdout_ctx.write_channel = MS_TRUE;
  stdout_ctx.readWriteFunc = msIO_apacheWrite;
  stdout_ctx.cbData = (void *)r;

  stderr_ctx.label = "apache";
  stderr_ctx.write_channel = MS_TRUE;
  stderr_ctx.readWriteFunc = msIO_apacheError;
  stderr_ctx.cbData = (void *)r;

  msIO_installHandlers(NULL, &stdout_ctx, &stderr_ctx);

  return MS_TRUE;
}

/* The maximum number of arguments we support. The number is taken from
 * cgiutil.h I think. It is only used for easier param decoding.
 */
#define WMS_MAX_ARGS 100

/* This is our simple query decoding function. Should be improved but for now
 * it works...
 */
static int mapserver_decode_args(apr_pool_t *p, char *args, char ***ParamNames,
                                 char ***ParamValues) {
  char **argv = NULL;
  int i;
  int n;
  int argc = 0;
  char *sep;

  /* alloc the name/value pointer list */
  argv = (char **)apr_pcalloc(p, (WMS_MAX_ARGS + 1) * 2 * sizeof(char *));
  *ParamNames = argv;
  *ParamValues = argv + WMS_MAX_ARGS + 1;
  /* No arguments? Then we're done */
  if (!args)
    return 0;

  argv[0] = args;

  /* separate the arguments */
  for (i = 1, n = 0; args[n] && (i < WMS_MAX_ARGS); n++)
    if (args[n] == '&') {
      argv[i++] = args + n + 1;
      args[n] = '\0';
    }

  /* eliminate empty args */
  for (n = 0, i = 0; argv[i]; i++)
    if (*(argv[i]) != '\0')
      argv[n++] = argv[i];
    else
      argv[i] = NULL;

  /* argument count is the number of non-zero arguments! */
  argc = n;

  /* split the name/value pairs */
  for (i = 0; argv[i]; i++) {
    sep = strchr(argv[i], '=');
    if (!sep)
      continue;
    *sep = '\0';
    argv[i + WMS_MAX_ARGS + 1] = (char *)apr_pstrdup(p, sep + 1);

    if (ap_unescape_url(argv[i + WMS_MAX_ARGS + 1]) == HTTP_BAD_REQUEST) {
      ap_log_error(APLOG_MARK, APLOG_ERR, 0, NULL,
                   "%s: malformed URI, couldn't unescape param %s", __func__,
                   argv[i]);

      argv[i + WMS_MAX_ARGS + 1] = NULL;
    } else {
      plustospace(argv[i + WMS_MAX_ARGS + 1]);
    }
  }
  return argc;
}

static char *mapserver_read_post_data(request_rec *r) {
  int size;
  long blen, rsize, rpos = 0;
  char *buffer = NULL;
  char buf[512];

  if (ap_setup_client_block(r, REQUEST_CHUNKED_ERROR) != OK)
    return NULL;

  if (!ap_should_client_block(r))
    return NULL;

  buffer = (char *)apr_palloc(r->pool, r->remaining + 1);

  if (!buffer)
    return NULL;

  size = r->remaining;
  buffer[size] = '\0';

  while ((blen = ap_get_client_block(r, buf, sizeof(buf))) > 0) {
    if (rpos + blen > size) {
      rsize = blen - rpos;
    } else {
      rsize = blen;
    }
    memcpy((char *)buffer + rpos, buf, rsize);
    rpos += rsize;
  }

  return buffer;
}

/*
** Extract Map File name from params and load it.
** Returns map object or NULL on error.
*/
static mapObj *msModuleLoadMap(mapservObj *mapserv,
                               mapserver_dir_config *conf) {
  int i;
  /* OK, here's the magic: we take the mapObj from our stored config.
   * We will use a copy of it created by msCopyMap since MapServer
   * modifies the object at several places during request processing
   */
  mapObj *map = msNewMapObj();
  if (!map)
    return NULL;
  msCopyMap(map, conf->map);

  /* check for any %variable% substitutions here, also do any map_ changes, we
   * do this here so WMS/WFS  */
  /* services can take advantage of these "vendor specific" extensions */
  for (i = 0; i < mapserv->request->NumParams; i++) {
    /*
    ** a few CGI variables should be skipped altogether
    **
    ** qstring: there is separate per layer validation for attribute queries and
    *the substitution checks
    **          below conflict with that so we avoid it here
    */
    if (strncasecmp(mapserv->request->ParamNames[i], "qstring", 7) == 0)
      continue;

    if (strncasecmp(mapserv->request->ParamNames[i], "map_", 4) == 0 ||
        strncasecmp(mapserv->request->ParamNames[i], "map.", 4) ==
            0) { /* check to see if there are any additions to the mapfile */
      if (msUpdateMapFromURL(map, mapserv->request->ParamNames[i],
                             mapserv->request->ParamValues[i]) != MS_SUCCESS) {
        msFreeMap(map);
        return NULL;
      }
      continue;
    }
  }

  msApplySubstitutions(map, mapserv->request->ParamNames,
                       mapserv->request->ParamValues,
                       mapserv->request->NumParams);
  msApplyDefaultSubstitutions(map);

  /* check to see if a ogc map context is passed as argument. if there */
  /* is one load it */

  for (i = 0; i < mapserv->request->NumParams; i++) {
    if (strcasecmp(mapserv->request->ParamNames[i], "context") == 0) {
      if (mapserv->request->ParamValues[i] &&
          strlen(mapserv->request->ParamValues[i]) > 0) {
        if (strncasecmp(mapserv->request->ParamValues[i], "http", 4) == 0) {
          if (msGetConfigOption(map, "CGI_CONTEXT_URL"))
            msLoadMapContextURL(map, mapserv->request->ParamValues[i],
                                MS_FALSE);
        } else
          msLoadMapContext(map, mapserv->request->ParamValues[i], MS_FALSE);
      }
    }
  }
  /*
   * RFC-42 HTTP Cookie Forwarding
   * Here we set the http_cookie_data metadata to handle the
   * HTTP Cookie Forwarding. The content of this metadata is the cookie
   * content. In the future, this metadata will probably be replaced
   * by an object that is part of the mapObject that would contain
   * information on the application status (such as cookie).
   */
  if (mapserv->request->httpcookiedata != NULL) {
    msInsertHashTable(&(map->web.metadata), "http_cookie_data",
                      mapserv->request->httpcookiedata);
  }

  return map;
}

/***
 * The main request handler.
 **/
static int mapserver_handler(request_rec *r) {
  /* acquire the appropriate configuration for this directory */
  mapserver_dir_config *conf;
  conf = (mapserver_dir_config *)ap_get_module_config(r->per_dir_config,
                                                      &mapserver_module);

  /* decline the request if there's no map configured */
  if (!conf || !conf->map)
    return DECLINED;

  apr_finfo_t mapstat;
  if (apr_stat(&mapstat, conf->mapfile_name, APR_FINFO_MTIME, r->pool) ==
      APR_SUCCESS) {
    if (apr_time_sec(mapstat.mtime) > apr_time_sec(conf->mtime)) {
      mapObj *newmap = msLoadMap(conf->mapfile_name, NULL);
      if (newmap) {
        msFreeMap(conf->map);
        conf->map = newmap;
        conf->mtime = mapstat.mtime;
      } else {
        ap_log_error(APLOG_MARK, APLOG_WARNING, 0, NULL,
                     "unable to reload map file %s", conf->mapfile_name);
      }
    }
  } else {
    ap_log_error(APLOG_MARK, APLOG_WARNING, 0, NULL,
                 "%s: unable to stat file %s", __func__, conf->mapfile_name);
  }

  /* make a copy of the URI so we can modify it safely */
  char *uri = apr_pstrdup(r->pool, r->uri);
  int len = strlen(uri);
  int conf_uri_len = strlen(conf->uri);

  /* If the URI points to a subdirectory we want to decline.
   */
  if (len > conf_uri_len)
    return DECLINED;

  int argc = 0;
  char **ParamNames = NULL;
  char **ParamValues = NULL;
  char *post_data = NULL;
  int szMethod = -1;
  char *szContentType = NULL;
  mapservObj *mapserv = NULL;

  /* Try decoding the query string */
  if (r->method_number == M_GET) {
    argc = mapserver_decode_args(r->pool, (char *)apr_pstrdup(r->pool, r->args),
                                 &ParamNames, &ParamValues);
    szMethod = MS_GET_REQUEST;
  } else if (r->method_number == M_POST) {
    szContentType = (char *)apr_table_get(r->headers_in, "Content-Type");
    post_data = mapserver_read_post_data(r);
    szMethod = MS_POST_REQUEST;
    if (strcmp(szContentType, "application/x-www-form-urlencoded") == 0) {
      argc =
          mapserver_decode_args(r->pool, (char *)apr_pstrdup(r->pool, r->args),
                                &ParamNames, &ParamValues);
    }
  } else
    return HTTP_METHOD_NOT_ALLOWED;

  if (!argc && !post_data)
    return HTTP_BAD_REQUEST;

  /* Now we install the IO redirection.
   */
  if (msIO_installApacheRedirect(r) != MS_TRUE)
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, NULL,
                 "%s: could not install apache redirect", __func__);

  mapserv = msAllocMapServObj();
  mapserv->request->NumParams = argc;
  mapserv->request->ParamNames = ParamNames;
  mapserv->request->ParamValues = ParamValues;
  mapserv->request->type = (enum MS_REQUEST_TYPE)szMethod;
  mapserv->request->postrequest = post_data;
  mapserv->request->contenttype = szContentType;

  // mapserv->map = msModuleLoadMap(mapserv,conf);
  mapserv->map = conf->map;
  if (!mapserv->map) {
    msCGIWriteError(mapserv);
    goto end_request;
  }

  if (msCGIDispatchRequest(mapserv) != MS_SUCCESS) {
    msCGIWriteError(mapserv);
    goto end_request;
  }

end_request:
  if (mapserv) {
    mapserv->request->ParamNames = NULL;
    mapserv->request->ParamValues = NULL;
    mapserv->request->postrequest = NULL;
    mapserv->request->contenttype = NULL;
    mapserv->map = NULL;
    msFreeMapServObj(mapserv);
  }
  msResetErrorList();

  /* Check if status was set inside MapServer functions. If it was, we
   * return it's value instead of simply OK. This is to support redirects
   * from maptemplate.c
   */
  if (r->status == HTTP_MOVED_TEMPORARILY)
    return r->status;

  return OK;
}

/* This function will be called on startup to allow us to register our
 * handler. We don't do anything else yet...
 */
static void mapserver_register_hooks(apr_pool_t *p) {
  ap_hook_handler(mapserver_handler, NULL, NULL, APR_HOOK_MIDDLE);
}

/* That's the second part of the overall magic. This function will be called
 * when the configuration reaches the "WMS_Map" parameter. It will be passed
 * in as argument and should contain the full path and name of the map file.
 * We try to load it and store the resulting mapObj in the config of the
 * current directory. If we fail we throw a message...
 */
static const char *mapserver_set_map(cmd_parms *cmd, void *config,
                                     const char *arg) {
  mapserver_dir_config *conf = (mapserver_dir_config *)config;
  /* if the mapObj already exists the WMS_Map was given more than once -
   * may be the user forgot to comment something out...
   */
  if (conf->map) {
    msWriteError(stderr);
    return (char *)apr_psprintf(cmd->temp_pool,
                                "An MAP-file has already been registered for "
                                "this URI - not accepting '%s'.",
                                arg);
  }
  /* Simply try loading the argument as map file. */
  conf->mapfile_name = apr_pstrdup(cmd->pool, arg);
  conf->map = msLoadMap((char *)arg, NULL);

  /* Ooops - we failed. We report it and fail. So beware: Always do a
   * configcheck before really restarting your web server!
   */

  if (!conf->map) {
    msWriteError(stderr);
    return (char *)apr_psprintf(
        cmd->temp_pool, "The given MAP-file '%s' could not be loaded", arg);
  }

  apr_finfo_t status;
  if (apr_stat(&status, conf->mapfile_name, APR_FINFO_MTIME, cmd->pool) !=
      APR_SUCCESS) {
    ap_log_error(APLOG_MARK, APLOG_WARNING, 0, NULL,
                 "%s: unable to stat file %s", __func__, conf->mapfile_name);
  }
  conf->mtime = status.mtime;
  return NULL;
}

/* This function creates the (default) directory configuration. Well, we have
 * no defaults to set so we simply alloc the memory...
 */
static void *mapserver_create_dir_config(apr_pool_t *p, char *dir) {
  mapserver_dir_config *newconf;
  newconf =
      (mapserver_dir_config *)apr_pcalloc(p, sizeof(mapserver_dir_config));
  newconf->config_pool = p;
  newconf->uri = apr_pstrdup(p, dir);
  newconf->map = NULL;
  newconf->mapfile_name = NULL;
  newconf->mtime = apr_time_from_sec(0);

  if (dir) {
    int len = strlen(dir);
    if (len > 1 && dir[len - 1] == '/')
      newconf->uri[len - 1] = '\0';
  }
  return newconf;
}

/* This structure defines our single config option which takes exactly one
 * argument.
 */
static const command_rec mapserver_options[] = {
    AP_INIT_TAKE1("Mapfile", mapserver_set_map, NULL, ACCESS_CONF,
                  "WMS_Map <Path to map file>"),
    {NULL}};

/* The following structure declares everything Apache needs to treat us as
 * a module. Merging would mean inheriting from the parent directory but that
 * doesn't make much sense here since we would only inherit the map file...
 */
/* Dispatch list for API hooks */
module AP_MODULE_DECLARE_DATA mapserver_module = {
    STANDARD20_MODULE_STUFF,
    mapserver_create_dir_config, /* create per-dir    config structures */
    NULL,                        /* merge  per-dir    config structures */
    NULL,                        /* create per-server config structures */
    NULL,                        /* merge  per-server config structures */
    mapserver_options,           /* table of config file commands       */
    mapserver_register_hooks     /* register hooks                      */
};

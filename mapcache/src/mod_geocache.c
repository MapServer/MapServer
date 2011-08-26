/*
 *  Copyright 2010 Thomas Bonfort
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/*
 * Include the core server components.
 */
#include <httpd.h>
#include <http_config.h>
#include <http_protocol.h>
#include <http_request.h>
#include <apr_strings.h>
#include <apr_time.h>
#include <http_log.h>
#include "geocache.h"

#ifdef AP_NEED_SET_MUTEX_PERMS
#include "unixd.h"
#endif 

static char* geocache_mutex_name = "geocache_mutex";


static int geocache_write_tile(request_rec *r, geocache_tile *tile) {
   int rc;


   ap_update_mtime(r, tile->mtime);
   if((rc = ap_meets_conditions(r)) != OK) {
      return rc;
   }
   ap_set_last_modified(r);
   ap_set_content_length(r,tile->data->size);
   if(tile->tileset->source->image_format == GEOCACHE_IMAGE_FORMAT_PNG) 
      ap_set_content_type(r, "image/png");
   else
      ap_set_content_type(r, "image/jpeg");
   ap_rwrite((void*)tile->data->buf, tile->data->size, r);

   return OK;
}

static int mod_geocache_request_handler(request_rec *r) {
   apr_table_t *params;
   geocache_cfg *config = NULL;
   geocache_request *request;
   geocache_tile *tile;
   int i;

   if (!r->handler || strcmp(r->handler, "geocache")) {
      return DECLINED;
   }
   if (r->method_number != M_GET) {
      return HTTP_METHOD_NOT_ALLOWED;
   }

   params = geocache_http_parse_param_string(r, r->args);
   config = ap_get_module_config(r->per_dir_config, &geocache_module);

   for(i=0;i<GEOCACHE_SERVICES_COUNT;i++) {
      /* loop through the services that have been configured */
      geocache_service *service = config->services[i];
      if(!service) continue;
      request = service->parse_request(r,params,config);
      /* the service has recognized the request if it returns a non NULL value */
      if(request)
         break;
   }
   if(!request || !request->ntiles) {
      return HTTP_BAD_REQUEST;
   }


   for(i=0;i<request->ntiles;i++) {
      geocache_tile *tile = request->tiles[i];
      int rv = geocache_tileset_tile_get(tile, r);
      if(rv != GEOCACHE_SUCCESS) {
         return HTTP_NOT_FOUND;
      }
   }
   if(request->ntiles == 1) {
      tile = request->tiles[0];
   } else {
      /* TODO: individual check on tiles if merging is allowed */

      tile = (geocache_tile*)geocache_image_merge_tiles(r,request->tiles,request->ntiles);
      if(!tile) {
         ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "tile merging failed to return data");
         return HTTP_INTERNAL_SERVER_ERROR;
      }
      tile->tileset = request->tiles[0]->tileset;
   }
   return geocache_write_tile(r,tile);
}

static int mod_geocache_post_config(apr_pool_t *p, apr_pool_t *plog, apr_pool_t *ptemp, server_rec *s) {
   apr_status_t rc;
   geocache_server_cfg* cfg = ap_get_module_config(s->module_config, &geocache_module);
#ifdef DEBUG
   if(!cfg) {
      ap_log_error(APLOG_MARK, APLOG_CRIT, 0, s, "configuration not found in server context");
      return 1;
   }
#endif
   rc = apr_global_mutex_create(&cfg->mutex,geocache_mutex_name,APR_LOCK_DEFAULT,p);
   if(rc != APR_SUCCESS) {
      ap_log_error(APLOG_MARK, APLOG_CRIT, rc, s, "Could not create global parent mutex %s", geocache_mutex_name);
      return rc;
   }
#ifdef AP_NEED_SET_MUTEX_PERMS
   rc = unixd_set_global_mutex_perms(cfg->mutex);
   if(rc != APR_SUCCESS) {
      ap_log_error(APLOG_MARK, APLOG_CRIT, rc, s, "Could not set permissions on global parent mutex %s", geocache_mutex_name);
      return rc;
   }
#endif
   apr_pool_cleanup_register(p,cfg->mutex,
         (void*)apr_global_mutex_destroy, apr_pool_cleanup_null);
   return OK;
}

static void mod_geocache_child_init(apr_pool_t *pool, server_rec *s) {
   geocache_server_cfg* cfg = ap_get_module_config(s->module_config, &geocache_module);
   apr_global_mutex_child_init(&(cfg->mutex),geocache_mutex_name, pool);
}

static void mod_geocache_register_hooks(apr_pool_t *p) {
   ap_hook_child_init(mod_geocache_child_init, NULL, NULL, APR_HOOK_MIDDLE);
   ap_hook_post_config(mod_geocache_post_config, NULL, NULL, APR_HOOK_MIDDLE);
   ap_hook_handler(mod_geocache_request_handler, NULL, NULL, APR_HOOK_LAST);
}

static void* mod_geocache_create_dir_conf(apr_pool_t *pool, char *x) {
   geocache_cfg *cfg = geocache_configuration_create(pool);
   return cfg;
}

static void* mod_geocache_create_server_conf(apr_pool_t *pool, server_rec *s) {
   geocache_server_cfg *cfg = apr_pcalloc(pool, sizeof(geocache_server_cfg));
   return cfg;
}



static const char* geocache_set_config_file(cmd_parms *cmd, void *cfg, const char *val) {
   geocache_cfg *config = (geocache_cfg*)cfg;
   char *msg = NULL;
   config->configFile = apr_pstrdup(cmd->pool,val);
   msg = geocache_configuration_parse(val,config,cmd->pool);
   ap_log_error(APLOG_MARK, APLOG_INFO, 0, cmd->server, "loaded geocache configuration file from %s", config->configFile);
   return msg;
}

static const command_rec mod_geocache_cmds[] = {
      AP_INIT_TAKE1("GeoCacheConfigFile", geocache_set_config_file,NULL,ACCESS_CONF,"Location of configuration file"),
      { NULL }
} ;

module AP_MODULE_DECLARE_DATA geocache_module =
{
      STANDARD20_MODULE_STUFF,
      mod_geocache_create_dir_conf,
      NULL,
      mod_geocache_create_server_conf,
      NULL,
      mod_geocache_cmds,
      mod_geocache_register_hooks
};

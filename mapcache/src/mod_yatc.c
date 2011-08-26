/*
	Copyright 2002 Kevin O'Donnell

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Include the core server components.
 */
#include <httpd.h>
#include <http_config.h>
#include <http_protocol.h>
#include <apr_strings.h>
#include <http_log.h>
#include "yatc.h"

#ifdef AP_NEED_SET_MUTEX_PERMS
#include "unixd.h"
#endif 

static char* yatc_mutex_name = "yatc_mutex";


static int yatc_write_tile(request_rec *r, yatc_tile *tile) {

   if(tile->tileset->source->image_format == YATC_IMAGE_FORMAT_PNG) 
      ap_set_content_type(r, "image/png");
   else
      ap_set_content_type(r, "image/jpeg");


   ap_set_content_length(r,tile->data->size);
   ap_rwrite((void*)tile->data->buf, tile->data->size, r);

   return OK;
}

static int mod_yatc_request_handler(request_rec *r) {
   apr_table_t *params;
   yatc_cfg *config = NULL;
   yatc_request *request;
   yatc_tile *tile;
   int i;

   if (!r->handler || strcmp(r->handler, "yatc")) {
      return DECLINED;
   }
   if (r->method_number != M_GET) {
      return HTTP_METHOD_NOT_ALLOWED;
   }

   params = yatc_http_parse_param_string(r, r->args);
   config = ap_get_module_config(r->per_dir_config, &yatc_module);

   for(i=0;i<YATC_SERVICES_COUNT;i++) {
      yatc_service *service = config->services[i];
      if(!service) continue;
      request = service->parse_request(r,params,config);
      if(request)
         break;
   }
   if(!request || !request->ntiles) {
      return HTTP_BAD_REQUEST;
   }


   for(i=0;i<request->ntiles;i++) {
      yatc_tile *tile = request->tiles[i];
      int rv = yatc_tileset_tile_get(tile, r);
      if(rv != YATC_SUCCESS) {
         return HTTP_NOT_FOUND;
      }
#ifdef MERGE_ENABLED
      if(request->ntiles>1) {
         if(tile->tileset->source->image_format != YATC_IMAGE_FORMAT_PNG) {
            ap_rprintf(r,"tile merging only supports png source (requested source: %s",tile->tileset->source->name);
            return HTTP_BAD_REQUEST;
         }
      }
#endif      
   }
   if(request->ntiles == 1) {
      tile = request->tiles[0];
   } else {
#ifdef MERGE_ENABLED
      /* TODO: individual check on tiles if merging is allowed */

      tile = (yatc_tile*)yatc_image_merge_tiles(r,request->tiles,request->ntiles);
      tile->tileset = request->tiles[0]->tileset;
#else
      ap_rputs("tile merging support not enabled on this server",r);
      return HTTP_BAD_REQUEST;
#endif
   }
   return yatc_write_tile(r,tile);
}

static int mod_yatc_post_config(apr_pool_t *p, apr_pool_t *plog, apr_pool_t *ptemp, server_rec *s) {
   apr_status_t rc;
   yatc_server_cfg* cfg = ap_get_module_config(s->module_config, &yatc_module);
#ifdef DEBUG
   if(!cfg) {
      ap_log_error(APLOG_MARK, APLOG_CRIT, 0, s, "configuration not found in server context");
      return 1;
   }
#endif
   rc = apr_global_mutex_create(&cfg->mutex,yatc_mutex_name,APR_LOCK_DEFAULT,p);
   if(rc != APR_SUCCESS) {
      ap_log_error(APLOG_MARK, APLOG_CRIT, rc, s, "Could not create global parent mutex %s", yatc_mutex_name);
      return rc;
   }
#ifdef AP_NEED_SET_MUTEX_PERMS
   rc = unixd_set_global_mutex_perms(cfg->mutex);
   if(rc != APR_SUCCESS) {
      ap_log_error(APLOG_MARK, APLOG_CRIT, rc, s, "Could not set permissions on global parent mutex %s", yatc_mutex_name);
      return rc;
   }
#endif
   apr_pool_cleanup_register(p,cfg->mutex,
         (void*)apr_global_mutex_destroy, apr_pool_cleanup_null);
   return OK;
}

static void mod_yatc_child_init(apr_pool_t *pool, server_rec *s) {
   yatc_server_cfg* cfg = ap_get_module_config(s->module_config, &yatc_module);
   apr_global_mutex_child_init(&(cfg->mutex),yatc_mutex_name, pool);
}

static void mod_yatc_register_hooks(apr_pool_t *p) {
   ap_hook_child_init(mod_yatc_child_init, NULL, NULL, APR_HOOK_MIDDLE);
   ap_hook_post_config(mod_yatc_post_config, NULL, NULL, APR_HOOK_MIDDLE);
   ap_hook_handler(mod_yatc_request_handler, NULL, NULL, APR_HOOK_LAST);
}

static void* mod_yatc_create_dir_conf(apr_pool_t *pool, char *x) {
   yatc_cfg *cfg = yatc_configuration_create(pool);
   return cfg;
}

static void* mod_yatc_create_server_conf(apr_pool_t *pool, server_rec *s) {
   yatc_server_cfg *cfg = apr_pcalloc(pool, sizeof(yatc_server_cfg));
   return cfg;
}



static const char* yatc_set_config_file(cmd_parms *cmd, void *cfg, const char *val) {
   yatc_cfg *config = (yatc_cfg*)cfg;
   char *msg = NULL;
   config->configFile = apr_pstrdup(cmd->pool,val);
   msg = yatc_configuration_parse(val,config,cmd->pool);
   ap_log_error(APLOG_MARK, APLOG_INFO, 0, cmd->server, "loaded yatc configuration file from %s", config->configFile);
   return msg;
}

static const command_rec mod_yatc_cmds[] = {
      AP_INIT_TAKE1("YatcConfigFile", yatc_set_config_file,NULL,ACCESS_CONF,"Location of configuration file"),
      { NULL }
} ;

module AP_MODULE_DECLARE_DATA yatc_module =
{
      STANDARD20_MODULE_STUFF,
      mod_yatc_create_dir_conf,
      NULL,
      mod_yatc_create_server_conf,
      NULL,
      mod_yatc_cmds,
      mod_yatc_register_hooks
};

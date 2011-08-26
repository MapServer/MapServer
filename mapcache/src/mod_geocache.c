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

module AP_MODULE_DECLARE_DATA geocache_module;


static char* geocache_mutex_name = "geocache_mutex";

typedef struct geocache_context_apache geocache_context_apache;
typedef struct geocache_context_apache_request geocache_context_apache_request;
typedef struct geocache_context_apache_server geocache_context_apache_server;


struct geocache_context_apache {
   geocache_context ctx;
};

struct geocache_context_apache_server {
   geocache_context_apache ctx;
   server_rec *server;
};

struct geocache_context_apache_request {
   geocache_context_apache ctx;
   request_rec *request;
};


void apache_context_set_error(geocache_context *c, geocache_error_code code, char *message, ...) {
   va_list args;
   va_start(args,message);
   c->_errmsg = apr_pvsprintf(c->pool,message,args);
   c->_errcode = code;
   va_end(args);
}


int apache_context_get_error(geocache_context *c) {
   return c->_errcode;
}

char* apache_context_get_error_message(geocache_context *c) {
   return c->_errmsg;
}

void apache_context_server_log(geocache_context *c, geocache_log_level level, char *message, ...) {
   geocache_context_apache_server *ctx = (geocache_context_apache_server*)c;
   va_list args;
   va_start(args,message);
   ap_log_error(APLOG_MARK, APLOG_INFO, 0, ctx->server,"%s",apr_pvsprintf(ctx->ctx.ctx.pool,message,args));
   va_end(args);
}

void apache_context_request_log(geocache_context *c, geocache_log_level level, char *message, ...) {
   geocache_context_apache_request *ctx = (geocache_context_apache_request*)c;
   va_list args;
   va_start(args,message);
   ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, ctx->request,"%s",apr_pvsprintf(ctx->ctx.ctx.pool,message,args));
   va_end(args);
}

void init_apache_context(geocache_context_apache *ctx) {
   ctx->ctx.set_error = apache_context_set_error;
   ctx->ctx.get_error = apache_context_get_error;
   ctx->ctx.get_error_message = apache_context_get_error_message;
}

int geocache_util_mutex_aquire(geocache_context *r, int nonblocking) {
   int ret;
   geocache_context_apache_request *ctx = (geocache_context_apache_request*)r;
   geocache_server_cfg *cfg = ap_get_module_config(ctx->request->server->module_config, &geocache_module);
   ret = apr_global_mutex_lock(cfg->mutex);
   if(ret != APR_SUCCESS) {
      ap_log_error(APLOG_MARK, APLOG_CRIT, 0, ctx->request->server, "failed to aquire mutex lock");
      return HTTP_INTERNAL_SERVER_ERROR;
   }
   apr_pool_cleanup_register(r->pool, cfg->mutex, (void*)apr_global_mutex_unlock, apr_pool_cleanup_null);
   return GEOCACHE_SUCCESS;
}

int geocache_util_mutex_release(geocache_context *r) {
   int ret;
   geocache_context_apache_request *ctx = (geocache_context_apache_request*)r;
   geocache_server_cfg *cfg = ap_get_module_config(ctx->request->server->module_config, &geocache_module);
   ret = apr_global_mutex_unlock(cfg->mutex);
   if(ret != APR_SUCCESS) {
      ap_log_error(APLOG_MARK, APLOG_CRIT, 0, ctx->request->server, "failed to release mutex");
      return HTTP_INTERNAL_SERVER_ERROR;
   }
   apr_pool_cleanup_kill(r->pool, cfg->mutex, (void*)apr_global_mutex_unlock);
   return GEOCACHE_SUCCESS;
}

void init_apache_request_context(geocache_context_apache_request *ctx) {
   init_apache_context((geocache_context_apache*)ctx);
   ctx->ctx.ctx.log = apache_context_request_log;
   ctx->ctx.ctx.global_lock_aquire = geocache_util_mutex_aquire;
   ctx->ctx.ctx.global_lock_release = geocache_util_mutex_release;
}

void init_apache_server_context(geocache_context_apache_server *ctx) {
   init_apache_context((geocache_context_apache*)ctx);
   ctx->ctx.ctx.log = apache_context_server_log;
   ctx->ctx.ctx.global_lock_aquire = geocache_util_mutex_aquire;
   ctx->ctx.ctx.global_lock_release = geocache_util_mutex_release;
}

static geocache_context_apache_request* apache_request_context_create(request_rec *r) {
   geocache_context_apache_request *ctx = apr_pcalloc(r->pool, sizeof(geocache_context_apache_request));
   ctx->ctx.ctx.pool = r->pool;
   ctx->request = r;
   init_apache_request_context(ctx);
   return ctx;
}

static geocache_context_apache_server* apache_server_context_create(server_rec *s, apr_pool_t *pool) {
   geocache_context_apache_server *ctx = apr_pcalloc(pool, sizeof(geocache_context_apache_server));
   ctx->ctx.ctx.pool = pool;
   ctx->server = s;
   init_apache_server_context(ctx);
   return ctx;
}




static int geocache_write_tile(geocache_context_apache_request *ctx, geocache_tile *tile) {
   int rc;
   request_rec *r = ctx->request;

   ap_update_mtime(r, tile->mtime);
   if((rc = ap_meets_conditions(r)) != OK) {
      return rc;
   }
   if(tile->expires) {
      apr_time_t now = apr_time_now();
      apr_time_t additional = apr_time_from_sec(tile->expires);
      apr_time_t expires = now + additional;
      apr_table_mergen(r->headers_out, "Cache-Control",apr_psprintf(r->pool, "max-age=%d", tile->expires));
      char *timestr = apr_palloc(r->pool, APR_RFC822_DATE_LEN);
      apr_rfc822_date(timestr, expires);
      apr_table_setn(r->headers_out, "Expires", timestr);

   }
   ap_set_last_modified(r);
   ap_set_content_length(r,tile->data->size);
   if(tile->tileset->format) {
      if(!strcmp(tile->tileset->format->extension,"png"))
         ap_set_content_type(r, "image/png");
      else
         ap_set_content_type(r, "image/jpeg");
   } else {
      geocache_image_format_type t = geocache_imageio_header_sniff((geocache_context*)ctx,tile->data);
      if(t == GC_PNG)
         ap_set_content_type(r, "image/png");
      else if(t == GC_JPEG)
         ap_set_content_type(r, "image/jpeg");
      else {
         ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "unrecognized image format");
         return HTTP_INTERNAL_SERVER_ERROR;
      }
   }
   ap_rwrite((void*)tile->data->buf, tile->data->size, r);

   return OK;
}

static int mod_geocache_request_handler(request_rec *r) {
   apr_table_t *params;
   geocache_cfg *config = NULL;
   geocache_request *request;
   geocache_context_apache_request *apache_ctx = apache_request_context_create(r); 
   geocache_context *global_ctx = (geocache_context*)apache_ctx;
   geocache_tile *tile;
   int i;

   if (!r->handler || strcmp(r->handler, "geocache")) {
      return DECLINED;
   }
   if (r->method_number != M_GET) {
      return HTTP_METHOD_NOT_ALLOWED;
   }

   params = geocache_http_parse_param_string(global_ctx, r->args);
   config = ap_get_module_config(r->per_dir_config, &geocache_module);

   for(i=0;i<GEOCACHE_SERVICES_COUNT;i++) {
      /* loop through the services that have been configured */
      geocache_service *service = config->services[i];
      if(!service) continue;
      request = service->parse_request(global_ctx,r->path_info,params,config);
      /* the service has recognized the request if it returns a non NULL value */
      if(request)
         break;
   }
   if(!request || !request->ntiles) {
      return HTTP_BAD_REQUEST;
   }


   for(i=0;i<request->ntiles;i++) {
      geocache_tile *tile = request->tiles[i];
      geocache_tileset_tile_get(global_ctx, tile);
      if(GC_HAS_ERROR(global_ctx)) {
         global_ctx->log(global_ctx,GEOCACHE_INFO,global_ctx->get_error_message(global_ctx));
         return HTTP_NOT_FOUND;
      }
   }
   if(request->ntiles == 1) {
      tile = request->tiles[0];
   } else {
      /* TODO: individual check on tiles if merging is allowed */

      tile = (geocache_tile*)geocache_image_merge_tiles(global_ctx,config->merge_format,request->tiles,request->ntiles);
      if(!tile) {
         ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "tile merging failed to return data");
         return HTTP_INTERNAL_SERVER_ERROR;
      }
      tile->tileset = request->tiles[0]->tileset;
   }
   return geocache_write_tile(apache_ctx,tile);
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
   geocache_context *ctx = (geocache_context*)apache_server_context_create(cmd->server,cmd->pool);
   char *msg = NULL;
   config->configFile = apr_pstrdup(cmd->pool,val);
   geocache_configuration_parse(ctx,val,config);
   if(GC_HAS_ERROR(ctx)) {
      return ctx->get_error_message(ctx);
   }
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

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
#include <http_core.h>
#include <http_config.h>
#include <http_protocol.h>
#include <http_request.h>
#include <apr_strings.h>
#include <apr_time.h>
#include <http_log.h>
#include "geocache.h"
#include <unistd.h>

#ifdef AP_NEED_SET_MUTEX_PERMS
#include "unixd.h"
#endif 

module AP_MODULE_DECLARE_DATA geocache_module;


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

int report_error(geocache_context_apache_request *apache_ctx) {
   geocache_context *ctx= (geocache_context*)apache_ctx;
   int code = ctx->_errcode;
   if(!code) code = 500;
   char *msg = ctx->get_error_message(ctx);
   if(!msg) {
      msg = "an unspecified error has occured";
   }
   ctx->log(ctx,GEOCACHE_INFO,msg);
   if(ctx->config && ctx->config->reporting == GEOCACHE_REPORT_MSG) {
      apache_ctx->request->status = code;
      ap_set_content_type(apache_ctx->request, "text/plain");
      ap_rprintf(apache_ctx->request,"error: %s",msg);
      return OK;
   } else if(ctx->config && ctx->config->reporting == GEOCACHE_REPORT_EMPTY_IMG) {
      if(ctx->config->empty_image) { /*should never fail*/
         apache_ctx->request->status = code;
         ap_set_content_length(apache_ctx->request, ctx->config->empty_image->size);
         apr_table_set(apache_ctx->request->headers_out, "X-Geocache-Error", msg);
         ap_set_content_type(apache_ctx->request, ctx->config->merge_format->mime_type);
         ap_rwrite((void*)ctx->config->empty_image->buf, ctx->config->empty_image->size, apache_ctx->request);
         return OK;
      } else {
         return code;
      }
   } else if(ctx->config && ctx->config->reporting == GEOCACHE_REPORT_ERROR_IMG) {
         apache_ctx->request->status = code;
         geocache_image *errim = geocache_error_image(ctx,256,256,msg);
         geocache_buffer *buf = ctx->config->merge_format->write(ctx,errim,ctx->config->merge_format);
         ap_set_content_length(apache_ctx->request, buf->size);
         apr_table_set(apache_ctx->request->headers_out, "X-Geocache-Error", msg);
         ap_set_content_type(apache_ctx->request, ctx->config->merge_format->mime_type);
         ap_rwrite((void*)buf->buf, buf->size, apache_ctx->request);
         return OK;
   }
   else {
      return code;
   }
}

void apache_context_server_log(geocache_context *c, geocache_log_level level, char *message, ...) {
   geocache_context_apache_server *ctx = (geocache_context_apache_server*)c;
   va_list args;
   va_start(args,message);
   char *msg = apr_pvsprintf(c->pool,message,args);
   va_end(args);
   ap_log_error(APLOG_MARK, APLOG_INFO, 0, ctx->server,"%s",msg);
}

void apache_context_request_log(geocache_context *c, geocache_log_level level, char *message, ...) {
   geocache_context_apache_request *ctx = (geocache_context_apache_request*)c;
   va_list args;
   va_start(args,message);
   ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, ctx->request, "%s", apr_pvsprintf(c->pool,message,args));
   va_end(args);
}

void geocache_util_mutex_aquire(geocache_context *gctx) {
   int ret;
   geocache_context_apache_request *ctx = (geocache_context_apache_request*)gctx;
   geocache_server_cfg *cfg = ap_get_module_config(ctx->request->server->module_config, &geocache_module);
   ret = apr_global_mutex_lock(cfg->mutex);
   if(ret != APR_SUCCESS) {
      gctx->set_error(gctx,500,"failed to lock mutex");
      return;
   }
   apr_pool_cleanup_register(gctx->pool, cfg->mutex, (void*)apr_global_mutex_unlock, apr_pool_cleanup_null);
}

void geocache_util_mutex_release(geocache_context *gctx) {
   int ret;
   geocache_context_apache_request *ctx = (geocache_context_apache_request*)gctx;
   geocache_server_cfg *cfg = ap_get_module_config(ctx->request->server->module_config, &geocache_module);
   ret = apr_global_mutex_unlock(cfg->mutex);
   if(ret != APR_SUCCESS) {
      gctx->set_error(gctx,500,"failed to unlock mutex");
   }
   apr_pool_cleanup_kill(gctx->pool, cfg->mutex, (void*)apr_global_mutex_unlock);
}

void init_apache_request_context(geocache_context_apache_request *ctx) {
   geocache_context_init((geocache_context*)ctx);
   ctx->ctx.ctx.log = apache_context_request_log;
   ctx->ctx.ctx.global_lock_aquire = geocache_util_mutex_aquire;
   ctx->ctx.ctx.global_lock_release = geocache_util_mutex_release;
}

void init_apache_server_context(geocache_context_apache_server *ctx) {
   geocache_context_init((geocache_context*)ctx);
   ctx->ctx.ctx.log = apache_context_server_log;
   ctx->ctx.ctx.global_lock_aquire = geocache_util_mutex_aquire;
   ctx->ctx.ctx.global_lock_release = geocache_util_mutex_release;
}

static geocache_context_apache_request* apache_request_context_create(request_rec *r) {
   geocache_context_apache_request *ctx = apr_pcalloc(r->pool, sizeof(geocache_context_apache_request));
   ctx->ctx.ctx.pool = r->pool;
   ctx->ctx.ctx.config = ap_get_module_config(r->per_dir_config, &geocache_module);
   ctx->request = r;
   init_apache_request_context(ctx);
   return ctx;
}

static geocache_context_apache_server* apache_server_context_create(server_rec *s, apr_pool_t *pool) {
   geocache_context_apache_server *ctx = apr_pcalloc(pool, sizeof(geocache_context_apache_server));
   ctx->ctx.ctx.pool = pool;
   ctx->ctx.ctx.config = NULL;
   ctx->server = s;
   init_apache_server_context(ctx);
   return ctx;
}




static int geocache_write_image_buffer(geocache_context_apache_request *ctx, geocache_buffer *im,
      geocache_image_format *format) {
   request_rec *r = ctx->request;

   ap_set_content_length(r,im->size);
   if(format && format->mime_type) {
      ap_set_content_type(r, format->mime_type);
   } else {
      geocache_image_format_type t = geocache_imageio_header_sniff((geocache_context*)ctx,im);
      if(t == GC_PNG)
         ap_set_content_type(r, "image/png");
      else if(t == GC_JPEG)
         ap_set_content_type(r, "image/jpeg");
      else {
         ctx->ctx.ctx.set_error((geocache_context*)ctx, 500, "unrecognized image format");
         return report_error(ctx);
      }
   }
   ap_rwrite((void*)im->buf, im->size, r);

   return OK;
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
      apr_table_set(r->headers_out, "Expires", timestr);
   }
   ap_set_last_modified(r);
   return geocache_write_image_buffer(ctx, tile->data, tile->tileset->format); 
}

static int geocache_write_capabilities(geocache_context_apache_request *ctx, geocache_request_get_capabilities *request) {
   request_rec *r = ctx->request;
   ap_set_content_type(r, request->mime_type);
   ap_rputs(request->capabilities, r);

   return OK;
}

static int geocache_write_featureinfo(geocache_context_apache_request *ctx, geocache_feature_info *fi) {
   request_rec *r = ctx->request;
   ap_set_content_type(r, fi->format);
   ap_set_content_length(r,fi->map.data->size);
   ap_rwrite((void*)fi->map.data->buf, fi->map.data->size, r);

   return OK;
}

static int mod_geocache_request_handler(request_rec *r) {
   apr_table_t *params;
   geocache_cfg *config = NULL;
   geocache_request *request = NULL;

   geocache_context_apache_request *apache_ctx = apache_request_context_create(r); 
   geocache_context *global_ctx = (geocache_context*)apache_ctx;
   int ret;

   if (!r->handler || strcmp(r->handler, "geocache")) {
      return DECLINED;
   }
   if (r->method_number != M_GET) {
      return HTTP_METHOD_NOT_ALLOWED;
   }
   
   

   params = geocache_http_parse_param_string(global_ctx, r->args);
   config = ap_get_module_config(r->per_dir_config, &geocache_module);

   geocache_service_dispatch_request(global_ctx,&request,r->path_info,params,config);
   if(GC_HAS_ERROR(global_ctx)) {
      return report_error(apache_ctx);
   }

   if(request->type == GEOCACHE_REQUEST_GET_CAPABILITIES) {
      geocache_request_get_capabilities *req_caps = (geocache_request_get_capabilities*)request;
      request_rec *original;
      char *url;
      if(r->main)
         original = r->main;
      else
         original = r;
      url = ap_construct_url(r->pool,original->uri,original);

      /*
       * remove the path_info from the end of the url (we want the url of the base of the service)
       * TODO: is there an apache api to access this ?
       */
      if(*(original->path_info)) {
         char *end = strstr(url,original->path_info);
         if(end) {
            *end = '\0';
         }
      }
      request->service->create_capabilities_response(global_ctx,req_caps,url,original->path_info,config);
      return geocache_write_capabilities(apache_ctx,req_caps);
   } else if( request->type == GEOCACHE_REQUEST_GET_TILE) {
      geocache_request_get_tile *req_tile = (geocache_request_get_tile*)request;
      if( !req_tile->ntiles) {
         return report_error(apache_ctx);
      }
      geocache_tile *tile = geocache_core_get_tile(global_ctx,req_tile);
      if(GC_HAS_ERROR(global_ctx)) {
         return report_error(apache_ctx);
      }
      ret = geocache_write_tile(apache_ctx,tile);
      return ret;

   } else if( request->type == GEOCACHE_REQUEST_GET_MAP) {
      geocache_request_get_map *req_map = (geocache_request_get_map*)request;
      geocache_map *map = geocache_core_get_map(global_ctx,req_map);
      if(GC_HAS_ERROR(global_ctx)) {
         return report_error(apache_ctx);
      }
      ret = geocache_write_image_buffer(apache_ctx,map->data, map->tileset->format);
      return ret;
   } else if( request->type == GEOCACHE_REQUEST_GET_FEATUREINFO) {
      geocache_request_get_feature_info *req_fi = (geocache_request_get_feature_info*)request;
      geocache_feature_info *fi = geocache_core_get_featureinfo(global_ctx,req_fi);
      if(GC_HAS_ERROR(global_ctx)) {
         return report_error(apache_ctx);
      }
      ret = geocache_write_featureinfo(apache_ctx, fi);
      return ret;
   } else {
      return report_error(apache_ctx);
   }
}

static int mod_geocache_post_config(apr_pool_t *p, apr_pool_t *plog, apr_pool_t *ptemp, server_rec *s) {
   apr_status_t rc;
   geocache_server_cfg* cfg = ap_get_module_config(s->module_config, &geocache_module);
   apr_lockmech_e lock_type = APR_LOCK_DEFAULT;

   if(!cfg) {
      ap_log_error(APLOG_MARK, APLOG_CRIT, 0, s, "configuration not found in server context");
      return 1;
   }

#if APR_HAS_PROC_PTHREAD_SERIALIZE
   lock_type = APR_LOCK_PROC_PTHREAD;
#endif
   rc = apr_global_mutex_create(&cfg->mutex,cfg->mutex_name,lock_type,p);
   if(rc != APR_SUCCESS) {
      ap_log_error(APLOG_MARK, APLOG_CRIT, rc, s, "Could not create global parent mutex %s", cfg->mutex_name);
      return rc;
   }
#ifdef AP_NEED_SET_MUTEX_PERMS
   rc = unixd_set_global_mutex_perms(cfg->mutex);
   if(rc != APR_SUCCESS) {
      ap_log_error(APLOG_MARK, APLOG_CRIT, rc, s, "Could not set permissions on global parent mutex %s", cfg->mutex_name);
      return rc;
   }
#endif
   apr_pool_cleanup_register(p,cfg->mutex,
         (void*)apr_global_mutex_destroy, apr_pool_cleanup_null);
#ifndef DISABLE_VERSION_STRING
   ap_add_version_component(p, GEOCACHE_USERAGENT);
#endif
   return OK;
}

static void mod_geocache_child_init(apr_pool_t *pool, server_rec *s) {
   geocache_server_cfg* cfg = ap_get_module_config(s->module_config, &geocache_module);
   apr_global_mutex_child_init(&(cfg->mutex),cfg->mutex_name, pool);
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
   char *mutex_unique_name = apr_psprintf(pool,"geocachemutex-%d",getpid());
   cfg->mutex_name = ap_server_root_relative(pool, mutex_unique_name);
   return cfg;
}


static void *mod_geocache_merge_server_conf(apr_pool_t *p, void *base_, void *vhost_)
{
   return base_;
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
      mod_geocache_merge_server_conf,
      mod_geocache_cmds,
      mod_geocache_register_hooks
};
/* vim: ai ts=3 sts=3 et sw=3
*/

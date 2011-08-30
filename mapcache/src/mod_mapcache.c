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
#include "mapcache.h"
#include <unistd.h>

#ifdef AP_NEED_SET_MUTEX_PERMS
#include "unixd.h"
#endif 

module AP_MODULE_DECLARE_DATA mapcache_module;


typedef struct mapcache_context_apache mapcache_context_apache;
typedef struct mapcache_context_apache_request mapcache_context_apache_request;
typedef struct mapcache_context_apache_server mapcache_context_apache_server;


struct mapcache_context_apache {
   mapcache_context ctx;
};

struct mapcache_context_apache_server {
   mapcache_context_apache ctx;
   server_rec *server;
};

struct mapcache_context_apache_request {
   mapcache_context_apache ctx;
   request_rec *request;
};

void apache_context_server_log(mapcache_context *c, mapcache_log_level level, char *message, ...) {
   mapcache_context_apache_server *ctx = (mapcache_context_apache_server*)c;
   va_list args;
   va_start(args,message);
   char *msg = apr_pvsprintf(c->pool,message,args);
   va_end(args);
   ap_log_error(APLOG_MARK, APLOG_INFO, 0, ctx->server,"%s",msg);
}

void apache_context_request_log(mapcache_context *c, mapcache_log_level level, char *message, ...) {
   mapcache_context_apache_request *ctx = (mapcache_context_apache_request*)c;
   va_list args;
   va_start(args,message);
   ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, ctx->request, "%s", apr_pvsprintf(c->pool,message,args));
   va_end(args);
}

void mapcache_util_mutex_aquire(mapcache_context *gctx) {
   int ret;
   mapcache_context_apache_request *ctx = (mapcache_context_apache_request*)gctx;
   mapcache_server_cfg *cfg = ap_get_module_config(ctx->request->server->module_config, &mapcache_module);
   ret = apr_global_mutex_lock(cfg->mutex);
   if(ret != APR_SUCCESS) {
      gctx->set_error(gctx,500,"failed to lock mutex");
      return;
   }
   apr_pool_cleanup_register(gctx->pool, cfg->mutex, (void*)apr_global_mutex_unlock, apr_pool_cleanup_null);
}

void mapcache_util_mutex_release(mapcache_context *gctx) {
   int ret;
   mapcache_context_apache_request *ctx = (mapcache_context_apache_request*)gctx;
   mapcache_server_cfg *cfg = ap_get_module_config(ctx->request->server->module_config, &mapcache_module);
   ret = apr_global_mutex_unlock(cfg->mutex);
   if(ret != APR_SUCCESS) {
      gctx->set_error(gctx,500,"failed to unlock mutex");
   }
   apr_pool_cleanup_kill(gctx->pool, cfg->mutex, (void*)apr_global_mutex_unlock);
}

void init_apache_request_context(mapcache_context_apache_request *ctx) {
   mapcache_context_init((mapcache_context*)ctx);
   ctx->ctx.ctx.log = apache_context_request_log;
   ctx->ctx.ctx.global_lock_aquire = mapcache_util_mutex_aquire;
   ctx->ctx.ctx.global_lock_release = mapcache_util_mutex_release;
}

void init_apache_server_context(mapcache_context_apache_server *ctx) {
   mapcache_context_init((mapcache_context*)ctx);
   ctx->ctx.ctx.log = apache_context_server_log;
   ctx->ctx.ctx.global_lock_aquire = mapcache_util_mutex_aquire;
   ctx->ctx.ctx.global_lock_release = mapcache_util_mutex_release;
}

static mapcache_context_apache_request* apache_request_context_create(request_rec *r) {
   mapcache_context_apache_request *ctx = apr_pcalloc(r->pool, sizeof(mapcache_context_apache_request));
   ctx->ctx.ctx.pool = r->pool;
   /* lookup the configuration object given the configuration file name */
   mapcache_server_cfg* cfg = ap_get_module_config(r->server->module_config, &mapcache_module);
   mapcache_cfg *config = apr_hash_get(cfg->aliases,(void*)r->filename,APR_HASH_KEY_STRING);
   ctx->ctx.ctx.config = config;
   ctx->request = r;
   init_apache_request_context(ctx);
   return ctx;
}

static mapcache_context_apache_server* apache_server_context_create(server_rec *s, apr_pool_t *pool) {
   mapcache_context_apache_server *ctx = apr_pcalloc(pool, sizeof(mapcache_context_apache_server));
   ctx->ctx.ctx.pool = pool;
   ctx->ctx.ctx.config = NULL;
   ctx->server = s;
   init_apache_server_context(ctx);
   return ctx;
}

static int write_http_response(mapcache_context_apache_request *ctx, mapcache_http_response *response) {
   request_rec *r = ctx->request;
   int rc;

   if(response->mtime) {
      ap_update_mtime(r, response->mtime);
      if((rc = ap_meets_conditions(r)) != OK) {
         return rc;
      }
      char *timestr = apr_palloc(r->pool, APR_RFC822_DATE_LEN);
      apr_rfc822_date(timestr, response->mtime);
      apr_table_setn(r->headers_out, "Last-Modified", timestr);
   }
   if(response->headers && !apr_is_empty_table(response->headers)) {
      const apr_array_header_t *elts = apr_table_elts(response->headers);
      int i;
      for(i=0;i<elts->nelts;i++) {
         apr_table_entry_t entry = APR_ARRAY_IDX(elts,i,apr_table_entry_t);
         if(!strcasecmp(entry.key,"Content-Type")) {
            ap_set_content_type(r,entry.val);
         } else {
            apr_table_set(r->headers_out, entry.key, entry.val);
         }
      }
   }
   if(response->data) {
      ap_set_content_length(r,response->data->size);
      ap_rwrite((void*)response->data->buf, response->data->size, r);
   }

   r->status = response->code;
   return OK;

}

static int mod_mapcache_request_handler(request_rec *r) {
   apr_table_t *params;
   mapcache_request *request = NULL;

   if (!r->handler || strcmp(r->handler, "mapcache")) {
      return DECLINED;
   }
   if (r->method_number != M_GET) {
      return HTTP_METHOD_NOT_ALLOWED;
   }
   
   
   mapcache_context_apache_request *apache_ctx = apache_request_context_create(r); 
   mapcache_context *global_ctx = (mapcache_context*)apache_ctx;

   params = mapcache_http_parse_param_string(global_ctx, r->args);

   mapcache_service_dispatch_request(global_ctx,&request,r->path_info,params,global_ctx->config);
   if(GC_HAS_ERROR(global_ctx) || !request) {
      return write_http_response(apache_ctx,
            mapcache_core_respond_to_error(global_ctx,(request)?request->service:NULL));
   }

   mapcache_http_response *http_response = NULL;

   if(request->type == MAPCACHE_REQUEST_GET_CAPABILITIES) {
      mapcache_request_get_capabilities *req_caps = (mapcache_request_get_capabilities*)request;
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
      if(*(original->path_info) && strcmp(original->path_info,"/")) {
         char *end = strstr(url,original->path_info);
         if(end) {
            /* make sure our url ends with a single '/' */
            if(*end == '/') {
               char *slash = end;
               while((*(--slash))=='/') end--;
               end++;
            }
            *end = '\0';
         }
      }
      http_response = mapcache_core_get_capabilities(global_ctx,request->service,req_caps,
            url,original->path_info,global_ctx->config);
   } else if( request->type == MAPCACHE_REQUEST_GET_TILE) {
      mapcache_request_get_tile *req_tile = (mapcache_request_get_tile*)request;
      http_response = mapcache_core_get_tile(global_ctx,req_tile);
   } else if( request->type == MAPCACHE_REQUEST_PROXY ) {
      mapcache_request_proxy *req_proxy = (mapcache_request_proxy*)request;
      http_response = mapcache_core_proxy_request(global_ctx, req_proxy);
   } else if( request->type == MAPCACHE_REQUEST_GET_MAP) {
      mapcache_request_get_map *req_map = (mapcache_request_get_map*)request;
      http_response = mapcache_core_get_map(global_ctx,req_map);
   } else if( request->type == MAPCACHE_REQUEST_GET_FEATUREINFO) {
      mapcache_request_get_feature_info *req_fi = (mapcache_request_get_feature_info*)request;
      http_response = mapcache_core_get_featureinfo(global_ctx,req_fi);
   } else {
      global_ctx->set_error(global_ctx,500,"###BUG### unknown request type");
   }

   if(GC_HAS_ERROR(global_ctx)) {
      return write_http_response(apache_ctx,
            mapcache_core_respond_to_error(global_ctx,request->service));
   }
   return write_http_response(apache_ctx,http_response);
}
static int mod_mapcache_post_config(apr_pool_t *p, apr_pool_t *plog, apr_pool_t *ptemp, server_rec *s) {
   apr_status_t rc;
   mapcache_server_cfg* cfg = ap_get_module_config(s->module_config, &mapcache_module);
   apr_lockmech_e lock_type = APR_LOCK_DEFAULT;
   server_rec *sconf;

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
   ap_add_version_component(p, MAPCACHE_USERAGENT);
#endif
   for (sconf = s->next; sconf; sconf = sconf->next) {
      mapcache_server_cfg* config = ap_get_module_config(sconf->module_config, &mapcache_module);
      config->mutex = cfg->mutex;
   }

#if APR_HAS_FORK
   /* fork a child process to let it accomplish post-configuration tasks with the uid of the runtime user */
   apr_proc_t proc;
   apr_status_t rv;
   rv = apr_proc_fork(&proc, ptemp);
   if (rv == APR_INCHILD) {
#define ap_unixd_setup_child unixd_setup_child
      ap_unixd_setup_child();
      mapcache_context *ctx = (mapcache_context*)apache_server_context_create(s,p);
      for (sconf = s; sconf; sconf = sconf->next) {
         mapcache_server_cfg* config = ap_get_module_config(sconf->module_config, &mapcache_module);
         if(config->aliases) {
            apr_hash_index_t *entry = apr_hash_first(ptemp,config->aliases);

            /* loop through the configured configurations */
            while (entry) {
               const char *alias;
               apr_ssize_t aliaslen;
               mapcache_cfg *c;
               apr_hash_this(entry,(const void**)&alias,&aliaslen,(void**)&c);
               mapcache_configuration_post_config(ctx, c);
               if(GC_HAS_ERROR(ctx)) {
                  ap_log_error(APLOG_MARK, APLOG_CRIT, APR_EGENERAL, s, "post config for %s failed: %s", alias,
                        ctx->get_error_message(ctx));
                  exit(APR_EGENERAL);
               }
               entry = apr_hash_next(entry);
            }
         }
      }
      exit(0);
   } else if (rv == APR_INPARENT) {
      apr_exit_why_e exitwhy;
      int exitcode;
      apr_proc_wait(&proc,&exitcode,&exitwhy,APR_WAIT);
      if(exitwhy != APR_PROC_EXIT) {
         ap_log_error(APLOG_MARK, APLOG_CRIT, APR_EGENERAL, s, "mapcache post-config child terminated abnormally");
         return APR_EGENERAL;
      } else {
         if(exitcode != 0) {
            return APR_EGENERAL;
         }
      }
      return OK;
   } else {
      ap_log_error(APLOG_MARK, APLOG_CRIT, APR_EGENERAL, s, "failed to fork mapcache post-config child");
      return APR_EGENERAL;
   }
#else /* APR_HAS_FORK */
   mapcache_context *ctx = (mapcache_context*)apache_server_context_create(s,p);
   for (sconf = s; sconf; sconf = sconf->next) {
      mapcache_server_cfg* config = ap_get_module_config(sconf->module_config, &mapcache_module);
      if(config->aliases) {
         apr_hash_index_t *entry = apr_hash_first(ptemp,config->aliases);

         /* loop through the configured configurations */
         while (entry) {
            const char *alias;
            apr_ssize_t aliaslen;
            mapcache_cfg *c;
            apr_hash_this(entry,(const void**)&alias,&aliaslen,(void**)&c);
            mapcache_configuration_post_config(ctx, c);
            if(GC_HAS_ERROR(ctx)) {
               ap_log_error(APLOG_MARK, APLOG_CRIT, APR_EGENERAL, s, "post config for %s failed: %s", alias,
                     ctx->get_error_message(ctx));
               return APR_EGENERAL;
            }
            entry = apr_hash_next(entry);
         }
      }
   }
   return OK;
#endif
}

static void mod_mapcache_child_init(apr_pool_t *pool, server_rec *s) {
   while(s) {
      mapcache_server_cfg* cfg = ap_get_module_config(s->module_config, &mapcache_module);
      apr_global_mutex_child_init(&(cfg->mutex),cfg->mutex_name, pool);
      s = s->next;
   }
}

static int mapcache_alias_matches(const char *uri, const char *alias_fakename)
{
    /* Code for this function from Apache mod_alias module. */

    const char *aliasp = alias_fakename, *urip = uri;

    while (*aliasp) {
        if (*aliasp == '/') {
            /* any number of '/' in the alias matches any number in
             * the supplied URI, but there must be at least one...
             */
            if (*urip != '/')
                return 0;

            do {
                ++aliasp;
            } while (*aliasp == '/');
            do {
                ++urip;
            } while (*urip == '/');
        }
        else {
            /* Other characters are compared literally */
            if (*urip++ != *aliasp++)
                return 0;
        }
    }

    /* Check last alias path component matched all the way */

    if (aliasp[-1] != '/' && *urip != '\0' && *urip != '/')
        return 0;

    /* Return number of characters from URI which matched (may be
     * greater than length of alias, since we may have matched
     * doubled slashes)
     */
    return urip - uri;
}

static int mapcache_hook_intercept(request_rec *r)
{
   mapcache_server_cfg *sconfig = ap_get_module_config(r->server->module_config, &mapcache_module);

   if (!sconfig->aliases)
      return DECLINED;

   if (r->uri[0] != '/' && r->uri[0])
      return DECLINED;

   apr_hash_index_t *entry = apr_hash_first(r->pool,sconfig->aliases);

   /* loop through the entries to find one where the alias matches */
   while (entry) {
      int l = 0;
      const char *alias;
      apr_ssize_t aliaslen;
      mapcache_cfg *c;
      apr_hash_this(entry,(const void**)&alias,&aliaslen,(void**)&c);

      if((l=mapcache_alias_matches(r->uri, c->endpoint))>0) {
         r->handler = "mapcache";
         r->filename = c->configFile;
         r->path_info = &(r->uri[l]);
         return OK;
      }

      entry = apr_hash_next(entry);
   }

   return DECLINED;
}


static void mod_mapcache_register_hooks(apr_pool_t *p) {
   ap_hook_child_init(mod_mapcache_child_init, NULL, NULL, APR_HOOK_MIDDLE);
   ap_hook_post_config(mod_mapcache_post_config, NULL, NULL, APR_HOOK_MIDDLE);
   ap_hook_handler(mod_mapcache_request_handler, NULL, NULL, APR_HOOK_MIDDLE);
   static const char * const p1[] = { "mod_alias.c", "mod_rewrite.c", NULL };
   static const char * const n1[]= { "mod_userdir.c",
                                      "mod_vhost_alias.c", NULL };
   ap_hook_translate_name(mapcache_hook_intercept, p1, n1, APR_HOOK_MIDDLE);

}

static void* mod_mapcache_create_server_conf(apr_pool_t *pool, server_rec *s) {
   mapcache_server_cfg *cfg = apr_pcalloc(pool, sizeof(mapcache_server_cfg));
   char *mutex_unique_name = apr_psprintf(pool,"mapcachemutex-%d",getpid());
   cfg->mutex_name = ap_server_root_relative(pool, mutex_unique_name);
   cfg->aliases = NULL;
   return cfg;
}


static void *mod_mapcache_merge_server_conf(apr_pool_t *p, void *base_, void *vhost_)
{
   mapcache_server_cfg *base = (mapcache_server_cfg*)base_;
   mapcache_server_cfg *vhost = (mapcache_server_cfg*)vhost_;
   mapcache_server_cfg *cfg = apr_pcalloc(p,sizeof(mapcache_server_cfg));
   cfg->mutex = base->mutex;
   cfg->mutex_name = base->mutex_name;
   
   if (base->aliases && vhost->aliases) {
      cfg->aliases = apr_hash_overlay(p, vhost->aliases,base->aliases);
   }
   else if (vhost->aliases) {
      cfg->aliases = apr_hash_copy(p,vhost->aliases);
   }
   else if (base->aliases) {
      cfg->aliases = apr_hash_copy(p,base->aliases);
   }
   return vhost;
}

static const char* mapcache_add_alias(cmd_parms *cmd, void *cfg, const char *alias, const char* configfile) {
   mapcache_server_cfg *sconfig = ap_get_module_config(cmd->server->module_config, &mapcache_module);
   mapcache_cfg *config = mapcache_configuration_create(cmd->pool);
   mapcache_context *ctx = (mapcache_context*)apache_server_context_create(cmd->server,cmd->pool);
   char *msg = NULL;
   config->configFile = apr_pstrdup(cmd->pool,configfile);
   config->endpoint = alias;
   mapcache_configuration_parse(ctx,configfile,config,0);
   if(GC_HAS_ERROR(ctx)) {
      return ctx->get_error_message(ctx);
   }
   ap_log_error(APLOG_MARK, APLOG_INFO, 0, cmd->server, "loaded mapcache configuration file from %s on alias %s", config->configFile, alias);
   if(!sconfig->aliases) {
      sconfig->aliases = apr_hash_make(cmd->pool);
   }
   apr_hash_set(sconfig->aliases,configfile,APR_HASH_KEY_STRING,config);
   return msg;
}



static const command_rec mod_mapcache_cmds[] = {
      AP_INIT_TAKE2("MapCacheAlias", mapcache_add_alias ,NULL,RSRC_CONF,"Aliased location of configuration file"),
      { NULL }
} ;

module AP_MODULE_DECLARE_DATA mapcache_module =
{
      STANDARD20_MODULE_STUFF,
      NULL,
      NULL,
      mod_mapcache_create_server_conf,
      mod_mapcache_merge_server_conf,
      mod_mapcache_cmds,
      mod_mapcache_register_hooks
};
/* vim: ai ts=3 sts=3 et sw=3
*/

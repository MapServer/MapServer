/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  MapCache tile caching CGI and FastCGI main program
 * Author:   Thomas Bonfort and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2011 Regents of the University of Minnesota.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

#include "mapcache.h"
#include <stdlib.h>
#include <apr_strings.h>
#include <apr_pools.h>
#include <apr_file_io.h>
#include <signal.h>
#include <apr_date.h>
#ifdef USE_FASTCGI
#include <fcgi_stdio.h>
#endif

typedef struct mapcache_context_fcgi mapcache_context_fcgi;
typedef struct mapcache_context_fcgi_request mapcache_context_fcgi_request;

static char *err400 = "Bad Request";
static char *err404 = "Not Found";
static char *err500 = "Internal Server Error";
static char *err501 = "Not Implemented";
static char *err502 = "Bad Gateway";
static char *errother = "No Description";
apr_pool_t *global_pool = NULL,*config_pool, *tmp_config_pool;

static char* err_msg(int code) {
   switch(code) {
      case 400:
         return err400;
      case 404:
         return err404;
      case 500:
         return err500;
      case 501:
         return err501;
      case 502:
         return err502;
      default:
         return errother;
   }
}

struct mapcache_context_fcgi {
   mapcache_context ctx;
   char *mutex_fname;
   apr_file_t *mutex_file;
};

static void fcgi_context_log(mapcache_context *c, mapcache_log_level level, char *message, ...) {
   va_list args;
   if(!c->config || level >= c->config->loglevel) {
      va_start(args,message);
      fprintf(stderr,"%s\n",apr_pvsprintf(c->pool,message,args));
      va_end(args);
   }
}

static void handle_signal(int signal) {
   apr_pool_destroy(global_pool);
   exit(signal);
}

static mapcache_context_fcgi* fcgi_context_create() {
   mapcache_context_fcgi *ctx = apr_pcalloc(global_pool, sizeof(mapcache_context_fcgi));
   if(!ctx) {
      return NULL;
   }
   ctx->ctx.pool = global_pool;
   mapcache_context_init((mapcache_context*)ctx);
   ctx->ctx.log = fcgi_context_log;
   ctx->mutex_fname="/tmp/mapcache.fcgi.lock";
   ctx->ctx.config = NULL;
   return ctx;
}

static void fcgi_write_response(mapcache_context_fcgi *ctx, mapcache_http_response *response) {
   if(response->code != 200) {
      printf("Status: %d %s\r\n",response->code, err_msg(response->code));
   }
   if(response->headers && !apr_is_empty_table(response->headers)) {
      const apr_array_header_t *elts = apr_table_elts(response->headers);
      int i;
      for(i=0;i<elts->nelts;i++) {
         apr_table_entry_t entry = APR_ARRAY_IDX(elts,i,apr_table_entry_t);
         printf("%s: %s\r\n", entry.key, entry.val);
      }
   }
   if(response->mtime) {
      char *if_modified_since = getenv("HTTP_IF_MODIFIED_SINCE");
      if(if_modified_since) {
        apr_time_t ims_time;
        apr_int64_t ims,mtime;


        mtime =  apr_time_sec(response->mtime);
        ims_time = apr_date_parse_http(if_modified_since);
        ims = apr_time_sec(ims_time);
        if(ims >= mtime) {
            printf("Status: 304 Not Modified\r\n");
        }
      }
      char *datestr = apr_palloc(ctx->ctx.pool, APR_RFC822_DATE_LEN);
      apr_rfc822_date(datestr, response->mtime);
      printf("Last-Modified: %s\r\n", datestr);
   }
   if(response->data) {
      printf("Content-Length: %d\r\n\r\n", response->data->size);
      fwrite((char*)response->data->buf, response->data->size,1,stdout);
   }
}


apr_time_t mtime;
char *conffile;

static void load_config(mapcache_context *ctx, char *filename) {
   apr_file_t *f;
   apr_finfo_t finfo;
   if((apr_file_open(&f, filename, APR_FOPEN_READ, APR_UREAD | APR_GREAD,
               global_pool)) == APR_SUCCESS) {
      apr_file_info_get(&finfo, APR_FINFO_MTIME, f);
      apr_file_close(f);
   } else {
      ctx->set_error(ctx,500,"failed to open config file %s",filename);
      return;
   }
   if(ctx->config) {
      //we already have a loaded configuration, check that the config file hasn't changed
      if(finfo.mtime > mtime) {
         ctx->log(ctx,MAPCACHE_INFO,"config file has changed, reloading");
      } else {
         return;
      }
   }
   mtime = finfo.mtime;

   /* either we have no config, or it has changed */

   mapcache_cfg *old_cfg = ctx->config;
   apr_pool_create(&tmp_config_pool,global_pool);

   mapcache_cfg *cfg = mapcache_configuration_create(tmp_config_pool);
   ctx->config = cfg;
   ctx->pool = tmp_config_pool;

   mapcache_configuration_parse(ctx,conffile,cfg,1);
   if(GC_HAS_ERROR(ctx)) goto failed_load;
   mapcache_configuration_post_config(ctx, cfg);
   if(GC_HAS_ERROR(ctx)) goto failed_load;

   /* no error, destroy the previous pool if we are reloading the config */
   if(config_pool) {
      apr_pool_destroy(config_pool);
   }
   config_pool = tmp_config_pool;

   return;

failed_load:
   /* we failed to load the config file */
   if(config_pool) {
      /* we already have a running configuration, keep it and only log the error to not
       * interrupt the already running service */
      ctx->log(ctx,MAPCACHE_ERROR,"failed to reload config file %s: %s", conffile,ctx->get_error_message(ctx));
      ctx->clear_errors(ctx);
      ctx->config = old_cfg;
      ctx->pool = config_pool;
      apr_pool_destroy(tmp_config_pool);
   }

}

int main(int argc, const char **argv) {
   (void) signal(SIGTERM,handle_signal);
   (void) signal(SIGUSR1,handle_signal);
   apr_initialize();
   atexit(apr_terminate);
   apr_pool_initialize();
   if(apr_pool_create(&global_pool,NULL) != APR_SUCCESS) {
      return 1;
   }
   config_pool = NULL;
   mapcache_context_fcgi* globalctx = fcgi_context_create();
   mapcache_context* ctx = (mapcache_context*)globalctx;
   
   conffile  = getenv("MAPCACHE_CONFIG_FILE");
   if(!conffile) {
      ctx->log(ctx,MAPCACHE_ERROR,"no config file found in MAPCACHE_CONFIG_FILE envirronement");
      return 1;
   }
   ctx->log(ctx,MAPCACHE_INFO,"mapcache fcgi conf file: %s",conffile);
   
   
#ifdef USE_FASTCGI
   while (FCGI_Accept() >= 0) {
#endif
      apr_table_t *params;
      ctx->pool = config_pool;
      if(!ctx->config || ctx->config->autoreload) {
         load_config(ctx,conffile);
         if(GC_HAS_ERROR(ctx)) {
            fcgi_write_response(globalctx, mapcache_core_respond_to_error(ctx,NULL));
            goto cleanup;
         }
      }
      apr_pool_create(&(ctx->pool),config_pool);
      mapcache_request *request = NULL;
      char *pathInfo = getenv("PATH_INFO");
      

      params = mapcache_http_parse_param_string(ctx, getenv("QUERY_STRING"));
      mapcache_service_dispatch_request(ctx,&request,pathInfo,params,ctx->config);
      if(GC_HAS_ERROR(ctx) || !request) {
         fcgi_write_response(globalctx, mapcache_core_respond_to_error(ctx,(request)?request->service:NULL));
         goto cleanup;
      }
      
      mapcache_http_response *http_response = NULL;
      if(request->type == MAPCACHE_REQUEST_GET_CAPABILITIES) {
         mapcache_request_get_capabilities *req = (mapcache_request_get_capabilities*)request;
         char *host = getenv("SERVER_NAME");
         char *port = getenv("SERVER_PORT");
         char *fullhost;
         char *url;
         if(getenv("HTTPS")) {
            if(!port || !strcmp(port,"443")) {
               fullhost = apr_psprintf(ctx->pool,"https://%s",host);
            } else {
               fullhost = apr_psprintf(ctx->pool,"https://%s:%s",host,port);
            }
         } else {
            if(!port || !strcmp(port,"80")) {
               fullhost = apr_psprintf(ctx->pool,"http://%s",host);
            } else {
               fullhost = apr_psprintf(ctx->pool,"http://%s:%s",host,port);
            }
         }
         url = apr_psprintf(ctx->pool,"%s%s/",
               fullhost,
               getenv("SCRIPT_NAME")
               );
         http_response = mapcache_core_get_capabilities(ctx,request->service,req,url,pathInfo,ctx->config);
      } else if( request->type == MAPCACHE_REQUEST_GET_TILE) {
         mapcache_request_get_tile *req_tile = (mapcache_request_get_tile*)request;
         http_response = mapcache_core_get_tile(ctx,req_tile);
      } else if( request->type == MAPCACHE_REQUEST_PROXY ) {
         mapcache_request_proxy *req_proxy = (mapcache_request_proxy*)request;
         http_response = mapcache_core_proxy_request(ctx, req_proxy);
      } else if( request->type == MAPCACHE_REQUEST_GET_MAP) {
         mapcache_request_get_map *req_map = (mapcache_request_get_map*)request;
         http_response = mapcache_core_get_map(ctx,req_map);
      } else if( request->type == MAPCACHE_REQUEST_GET_FEATUREINFO) {
         mapcache_request_get_feature_info *req_fi = (mapcache_request_get_feature_info*)request;
         http_response = mapcache_core_get_featureinfo(ctx,req_fi);
#ifdef DEBUG
      } else {
         ctx->set_error(ctx,500,"###BUG### unknown request type");
#endif
      }
      if(GC_HAS_ERROR(ctx)) {
         fcgi_write_response(globalctx, mapcache_core_respond_to_error(ctx,request->service));
         goto cleanup;
      }
#ifdef DEBUG
      if(!http_response) {
         ctx->set_error(ctx,500,"###BUG### NULL response");
         fcgi_write_response(globalctx, mapcache_core_respond_to_error(ctx,request->service));
         goto cleanup;
      }
#endif
      fcgi_write_response(globalctx,http_response);
cleanup:
#ifdef USE_FASTCGI
      apr_pool_destroy(ctx->pool);
      ctx->clear_errors(ctx);
   }
#endif
   apr_pool_destroy(global_pool);
   return 0;

}
/* vim: ai ts=3 sts=3 et sw=3
*/

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

#include "geocache.h"
#include <fcgiapp.h>
#include <stdlib.h>
#include <apr_strings.h>
#include <apr_pools.h>
#include <apr_file_io.h>

typedef struct geocache_context_fcgi geocache_context_fcgi;
typedef struct geocache_context_fcgi_request geocache_context_fcgi_request;

struct geocache_context_fcgi {
   geocache_context ctx;
   char *mutex_fname;
   apr_file_t *mutex_file;
};

struct geocache_context_fcgi_request {
   geocache_context_fcgi ctx;
   FCGX_Stream *out;
   FCGX_Stream *err;
};

void fcgi_context_log(geocache_context *c, geocache_log_level level, char *message, ...) {
   va_list args;
   va_start(args,message);
   fprintf(stderr,"%s\n",apr_pvsprintf(c->pool,message,args));
}

void fcgi_request_context_log(geocache_context *c, geocache_log_level level, char *message, ...) {
   va_list args;
   va_start(args,message);
   FCGX_FPrintF(((geocache_context_fcgi_request*)c)->err,"%s\n",apr_pvsprintf(c->pool,message,args));
}

void init_fcgi_context(geocache_context_fcgi *ctx) {
   geocache_context_init((geocache_context*)ctx);
   ctx->ctx.log = fcgi_context_log;
   ctx->mutex_fname="/tmp/geocache.fcgi.lock";
}

void geocache_fcgi_mutex_aquire(geocache_context *gctx) {
   geocache_context_fcgi *ctx = (geocache_context_fcgi*)gctx;
   int ret;
#ifdef DEBUG
   if(ctx->mutex_file != NULL) {
      gctx->set_error(gctx, GEOCACHE_DISK_ERROR, "SEVERE: fcgi recursive mutex acquire");
      return; /* BUG ! */
   }
#endif
   if (apr_file_open(&ctx->mutex_file, ctx->mutex_fname,
             APR_FOPEN_CREATE | APR_FOPEN_WRITE | APR_FOPEN_SHARELOCK | APR_FOPEN_BINARY,
             APR_OS_DEFAULT, gctx->pool) != APR_SUCCESS) {
      gctx->set_error(gctx, GEOCACHE_DISK_ERROR, "failed to create fcgi mutex lockfile %s", ctx->mutex_fname);
      return; /* we could not create the file */
   }
   ret = apr_file_lock(ctx->mutex_file, APR_FLOCK_EXCLUSIVE);
   if (ret != APR_SUCCESS) {
      gctx->set_error(gctx, GEOCACHE_DISK_ERROR, "failed to lock fcgi mutex file %s", ctx->mutex_fname);
      return;
   }
}

void geocache_fcgi_mutex_release(geocache_context *gctx) {
   int ret;
   geocache_context_fcgi *ctx = (geocache_context_fcgi*)gctx;
#ifdef DEBUG
   if(ctx->mutex_file == NULL) {
      gctx->set_error(gctx, GEOCACHE_DISK_ERROR, "SEVERE: fcgi mutex unlock on unlocked file");
      return; /* BUG ! */
   }
#endif
   ret = apr_file_unlock(ctx->mutex_file);
   if(ret != APR_SUCCESS) {
      gctx->set_error(gctx, GEOCACHE_DISK_ERROR,  "failed to unlock fcgi mutex file%s",ctx->mutex_fname);
   }
   ret = apr_file_close(ctx->mutex_file);
   if(ret != APR_SUCCESS) {
      gctx->set_error(gctx, GEOCACHE_DISK_ERROR,  "failed to close fcgi mutex file %s",ctx->mutex_fname);
   }
   ctx->mutex_file = NULL;
}

void init_fcgi_request_context(geocache_context_fcgi_request *ctx) {
   init_fcgi_context((geocache_context_fcgi*)ctx);
   ctx->ctx.ctx.global_lock_aquire = geocache_fcgi_mutex_aquire;
   ctx->ctx.ctx.global_lock_release = geocache_fcgi_mutex_release;
}

static geocache_context_fcgi* fcgi_context_create() {
   apr_pool_t *pool;
   if(apr_pool_create_core(&pool) != APR_SUCCESS) {
      return NULL;
   }
   geocache_context_fcgi *ctx = apr_pcalloc(pool, sizeof(geocache_context_fcgi));
   if(!ctx) {
      return NULL;
   }
   ctx->ctx.pool = pool;
   init_fcgi_context(ctx);
   return ctx;
}

static geocache_context_fcgi_request* fcgi_context_request_create(geocache_context_fcgi *parent, FCGX_Stream *out, FCGX_Stream *err) {
   apr_pool_t *pool;
   apr_pool_create(&pool,parent->ctx.pool);
   geocache_context_fcgi_request *ctx = apr_pcalloc(pool, sizeof(geocache_context_fcgi_request));
   ctx->ctx.ctx.pool = pool;
   ctx->ctx.mutex_fname = parent->mutex_fname;
   ctx->out = out;
   ctx->err = err;
   init_fcgi_request_context(ctx);
   return ctx;
}



static int geocache_write_tile(geocache_context_fcgi_request *ctx, geocache_tile *tile) {
   if(tile->tileset->format) {
      if(!strcmp(tile->tileset->format->extension,"png"))
         FCGX_FPrintF(ctx->out,"Content-type: image/png\r\n\r\n");
      else
         FCGX_FPrintF(ctx->out,"Content-type: image/jpeg\r\n\r\n");
   } else {
      geocache_image_format_type t = geocache_imageio_header_sniff((geocache_context*)ctx,tile->data);
      if(t == GC_PNG)
         FCGX_FPrintF(ctx->out,"Content-type: image/png\r\n\r\n");
      else if(t == GC_JPEG)
         FCGX_FPrintF(ctx->out,"Content-type: image/jpeg\r\n\r\n");
      else {
         return 500;
      }
   }
   FCGX_PutStr((char*)tile->data->buf, tile->data->size,ctx->out);

   return 200;
}

static int geocache_write_capabilities(geocache_context_fcgi_request *ctx, geocache_request *request) {
   FCGX_FPrintF(ctx->out,"Content-type: text/xml\r\n\r\n");
   FCGX_PutStr(request->capabilities, strlen(request->capabilities),ctx->out);

   return 200;
}

int main(int argc, char **argv) {
   
   apr_pool_initialize();
   geocache_context_fcgi* globalctx = fcgi_context_create();
   geocache_context* c = (geocache_context*)globalctx;
   geocache_cfg *cfg = geocache_configuration_create(c->pool);
   FCGX_Stream *in, *out, *err;
   FCGX_ParamArray envp;
   
   FCGX_Init();

   char *conffile  = getenv("GEOCACHE_CONFIG_FILE");
   if(!conffile) {
      c->log(c,GEOCACHE_ERROR,"no config file found in GEOCACHE_CONFIG_FILE envirronement");
      return 1;
   }
   c->log(c,GEOCACHE_DEBUG,"geocache fcgi conf file: %s",conffile);
   geocache_configuration_parse(c,conffile,cfg);
   if(GC_HAS_ERROR(c)) {
      c->log(c,GEOCACHE_ERROR,"failed to parse %s: %s",conffile,c->get_error_message(c));
      return 1;
   }
   
   while (FCGX_Accept(&in, &out, &err, &envp) >= 0) {
      apr_table_t *params;
      geocache_context *ctx = (geocache_context*) fcgi_context_request_create(globalctx,out,err); 
      geocache_tile *tile;
      geocache_request *request = NULL;
      char *host = FCGX_GetParam("SERVER_NAME",envp);
      char *port = FCGX_GetParam("SERVER_PORT",envp);
      char *fullhost;
      if(FCGX_GetParam("HTTPS",envp)) {
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
      char *pathInfo = FCGX_GetParam("PATH_INFO",envp);
      char *uri = apr_psprintf(ctx->pool,"%s%s",
            fullhost,
            FCGX_GetParam("SCRIPT_NAME",envp)
            );

      int i;


      params = geocache_http_parse_param_string((geocache_context*)ctx, FCGX_GetParam("QUERY_STRING",envp));

      for(i=0;i<GEOCACHE_SERVICES_COUNT;i++) {
         geocache_service *service = cfg->services[i];
         if(!service) continue;
         request = service->parse_request(ctx,uri,pathInfo,params,cfg);
         if(request)
            break;
      }
      if(!request || GC_HAS_ERROR(ctx)) {
         FCGX_FPrintF(out,"Status: 404 Not Found\r\n\r\n");
         goto cleanup;
      }
      if(request->type == GEOCACHE_REQUEST_GET_CAPABILITIES) {
         geocache_write_capabilities((geocache_context_fcgi_request*)ctx,request);
      } else {
         if(!request->ntiles) {
            FCGX_FPrintF(out,"Status: 404 Not Found\r\n\r\n");
            goto cleanup;
         }
         for(i=0;i<request->ntiles;i++) {
            geocache_tile *tile = request->tiles[i];
            geocache_tileset_tile_get(ctx,tile);
            if(GC_HAS_ERROR(ctx)) {
               ctx->log(ctx,GEOCACHE_DEBUG,ctx->get_error_message(ctx));
               FCGX_FPrintF(out,"Status: 500 Internal Server Error\r\n\r\n");
               goto cleanup;
            }
         }
         if(request->ntiles == 1) {
            tile = request->tiles[0];
         } else {
            tile = geocache_image_merge_tiles(ctx,cfg->merge_format,request->tiles,request->ntiles);
            if(!tile) {
               ctx->log(ctx,GEOCACHE_ERROR, "tile merging failed to return data");
               if(ctx->get_error(ctx)) {
                  ctx->log(ctx,GEOCACHE_ERROR,c->get_error_message(ctx));
                  FCGX_FPrintF(out,"Status: 500 Internal Server Error\r\n\r\n");
                  goto cleanup;
               }
               FCGX_FPrintF(out,"Status: 500 Internal Server Error\r\n\r\n");
               goto cleanup;
            }
            tile->tileset = request->tiles[0]->tileset;
         }
         geocache_write_tile((geocache_context_fcgi_request*)ctx,tile);
      }
cleanup:
      apr_pool_destroy(ctx->pool);
   }
   apr_pool_destroy(globalctx->ctx.pool);
   return 0;

}

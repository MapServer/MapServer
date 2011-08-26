#include "geocache.h"
#include <apr_thread_proc.h>
#include <apr_thread_mutex.h>
#include <apr_getopt.h>
#include <signal.h>
#include <time.h>
#include <apr_time.h>

#ifdef USE_OGR
#include "ogr_api.h"
int nClippers = 0;
OGRGeometryH *clippers=NULL;
#endif

typedef struct geocache_context_seeding geocache_context_seeding;
struct geocache_context_seeding{
    geocache_context ctx;
    int (*get_next_tile)(geocache_context_seeding *ctx, geocache_tile *tile, geocache_context *tmpctx);
    apr_thread_mutex_t *mutex;
    geocache_tileset *tileset;
    apr_table_t *dimensions;
    int minzoom;
    int maxzoom;
    int nextx,nexty,nextz;
    geocache_grid_link *grid_link;
};

int sig_int_received = 0;

static const apr_getopt_option_t seed_options[] = {
    /* long-option, short-option, has-arg flag, description */
    { "config", 'c', TRUE, "configuration file (/path/to/geocacahe.xml)"},
    { "tileset", 't', TRUE, "tileset to seed" },
    { "grid", 'g', TRUE, "grid to seed" },
    { "zoom", 'z', TRUE, "min and max zoomlevels to seed, separated by a comma. eg 0,6" },
    { "extent", 'e', TRUE, "extent to seed, format: minx,miny,maxx,maxy" },
    { "nthreads", 'n', TRUE, "number of parallel threads to use" },
    { "older", 'o', TRUE, "reseed tiles older than supplied date (format: year/month/day hour:minute, eg: 2011/01/31 20:45" },
    { "dimension", 'D', TRUE, "set the value of a dimension (format DIMENSIONNAME=VALUE). Can be used multiple times for multiple dimensions" },
#ifdef USE_OGR
    { "ogr-datasource", 'd', TRUE, "ogr datasource to get features from"},
    { "ogr-layer", 'l', TRUE, "layer inside datasource"},
    { "ogr-sql", 's', TRUE, "sql to filter inside layer"},
    { "ogr-where", 'w', TRUE, "filter to apply on layer features"},
#endif
    { "help", 'h', FALSE, "show help" },
    { NULL, 0, 0, NULL },
};

void handle_sig_int(int signal) {
    if(!sig_int_received) {
        fprintf(stderr,"SIGINT received, waiting for threads to finish\n");
        fprintf(stderr,"press ctrl-C again to force terminate, you might end up with locked tiles\n");
        sig_int_received = 1;
    } else {
        exit(signal);
    }
}

void geocache_context_seeding_lock_aquire(geocache_context *gctx) {
    int ret;
    geocache_context_seeding *ctx = (geocache_context_seeding*)gctx;
    ret = apr_thread_mutex_lock(ctx->mutex);
    if(ret != APR_SUCCESS) {
        gctx->set_error(gctx,500, "failed to lock mutex");
        return;
    }
    apr_pool_cleanup_register(gctx->pool, ctx->mutex, (void*)apr_thread_mutex_unlock, apr_pool_cleanup_null);
}

void geocache_context_seeding_lock_release(geocache_context *gctx) {
    int ret;
    geocache_context_seeding *ctx = (geocache_context_seeding*)gctx;
    ret = apr_thread_mutex_unlock(ctx->mutex);
    if(ret != APR_SUCCESS) {
        gctx->set_error(gctx,500, "failed to unlock mutex");
        return;
    }
    apr_pool_cleanup_kill(gctx->pool, ctx->mutex, (void*)apr_thread_mutex_unlock);
}

void geocache_context_seeding_log(geocache_context *ctx, geocache_log_level level, char *msg, ...) {
    /*va_list args;
    va_start(args,msg);
    vfprintf(stderr,msg,args);
    va_end(args);*/
   /* do nothing */
}

#ifdef USE_OGR
int ogr_features_intersect_tile(geocache_context *ctx, geocache_tile *tile) {
   geocache_metatile *mt = geocache_tileset_metatile_get(ctx,tile);
   OGRGeometryH mtbboxls = OGR_G_CreateGeometry(wkbLinearRing);
   double *e = mt->map.extent;
   OGR_G_SetPoint_2D(mtbboxls,0,e[0],e[1]);
   OGR_G_SetPoint_2D(mtbboxls,1,e[2],e[1]);
   OGR_G_SetPoint_2D(mtbboxls,2,e[2],e[3]);
   OGR_G_SetPoint_2D(mtbboxls,3,e[0],e[3]);
   OGR_G_SetPoint_2D(mtbboxls,4,e[0],e[1]);
   OGRGeometryH mtbbox = OGR_G_CreateGeometry(wkbPolygon);
   OGR_G_AddGeometryDirectly(mtbbox,mtbboxls);
   int i;
   for(i=0;i<nClippers;i++) {
      OGRGeometryH clipper = clippers[i];
      OGRGeometryH clipresult;
      clipresult = OGR_G_Intersection(mtbbox,clipper);
      if(clipresult && !OGR_G_IsEmpty(clipresult))
         return 1;
   }
   return 0;


}

#endif

apr_time_t age_limit = 0;
int should_seed_tile(geocache_context_seeding *sctx, int x, int y, int z,geocache_context *tmpctx) {
    geocache_context *ctx = (geocache_context*)sctx;
    geocache_tile *tile = geocache_tileset_tile_create(tmpctx->pool,sctx->tileset,sctx->grid_link);
    const apr_array_header_t* dimelts = apr_table_elts(sctx->dimensions); 	
    int i = dimelts->nelts;
    while(i--) {
       apr_table_entry_t *entry = &(APR_ARRAY_IDX(dimelts,i,apr_table_entry_t));
       apr_table_setn(tile->dimensions,entry->key,entry->val);
    }

    tile->x = x;
    tile->y = y;
    tile->z = z;
    int should_seed = sctx->tileset->cache->tile_exists(tmpctx,tile)?0:1;
    int intersects = -1;
    /* if the tile exists and a time limit was specified, check the tile modification date */
    if(!should_seed && age_limit) {
       if(sctx->tileset->cache->tile_get(tmpctx,tile) == GEOCACHE_SUCCESS) {
         if(tile->mtime && tile->mtime<age_limit) {
#ifdef USE_OGR
            /* check we are in the requested features before deleting the tile */
            if(nClippers > 0) {
               intersects = ogr_features_intersect_tile(ctx,tile);
            }
#endif
            if(intersects != 0) {
               /* either the tile intersects the ogr features, or we don't care about them:
                * delete the tile and recreate it */
               geocache_tileset_tile_delete(tmpctx,tile);
               return 1;
            } else {
               /* the tile does not intersect the ogr features, and already exists, do nothing */
               return 0;
            }
         }
       }
    }

    if(!should_seed)
       return 0;

    /* if here, the tile does not exist */
#ifdef USE_OGR
    /* check we are in the requested features before deleting the tile */
    if(nClippers > 0) {
       intersects = ogr_features_intersect_tile(ctx,tile);
    }
#endif
    if(intersects!= 0) {
      should_seed = 1;
    } else {
       should_seed = 0;
    }

    return should_seed;
}

int curz;
int seededtilestot, seededtiles;
time_t lastlogtime,starttime;


int geocache_context_seeding_get_next_tile(geocache_context_seeding *ctx, geocache_tile *tile, geocache_context *tmpcontext) {
    geocache_context *gctx= (geocache_context*)ctx;

    gctx->global_lock_aquire(gctx);
    if(ctx->nextz > ctx->maxzoom) {
       printf("this thread has finished seeding\n");
        gctx->global_lock_release(gctx);
        return GEOCACHE_FAILURE;
        //we have no tiles left to process
    }
    
    tile->x = ctx->nextx;
    tile->y = ctx->nexty;
    tile->z = ctx->nextz;


    while(1) {
        ctx->nextx += ctx->tileset->metasize_x;
        if(ctx->nextx > tile->grid_link->grid_limits[ctx->nextz][2]) {
            ctx->nexty += ctx->tileset->metasize_y;
            if(ctx->nexty >= tile->grid_link->grid_limits[ctx->nextz][3]) {
                ctx->nextz += 1;
                if(ctx->nextz > ctx->maxzoom) break;
                ctx->nexty = tile->grid_link->grid_limits[ctx->nextz][1];
            }
            ctx->nextx = tile->grid_link->grid_limits[ctx->nextz][0];
        }
        if(should_seed_tile(ctx, ctx->nextx, ctx->nexty, ctx->nextz, tmpcontext)){
           break;
        }
    }
    gctx->global_lock_release(gctx);
    return GEOCACHE_SUCCESS;
}

void geocache_context_seeding_init(geocache_context_seeding *ctx,
        geocache_cfg *cfg,
        geocache_tileset *tileset,
        int minzoom, int maxzoom,
        geocache_grid_link *grid_link,
        apr_table_t *dimensions) {
    int ret;
    geocache_context *gctx = (geocache_context*)ctx;
    geocache_context_init(gctx);
    ret = apr_thread_mutex_create(&ctx->mutex,APR_THREAD_MUTEX_DEFAULT,gctx->pool);
    if(ret != APR_SUCCESS) {
        gctx->set_error(gctx,500,"failed to create mutex");
        return;
    }

    /* validate the supplied dimensions */
    if (!apr_is_empty_table(dimensions)) {
       int i;
       for(i=0;i<tileset->dimensions->nelts;i++) {
          geocache_dimension *dimension = APR_ARRAY_IDX(tileset->dimensions,i,geocache_dimension*);
          const char *value;
          if((value = (char*)apr_table_get(dimensions,dimension->name)) != NULL) {
             char *tmpval = apr_pstrdup(gctx->pool,value);
             int ok = dimension->validate(gctx,dimension,&tmpval);
             GC_CHECK_ERROR(gctx);
             if(ok == GEOCACHE_SUCCESS) {
                /* validate may have changed the dimension value, so set it back into the dimensions table */
                apr_table_setn(dimensions,dimension->name,tmpval);
             }
             else {
                gctx->set_error(gctx,500,"dimension \"%s\" value \"%s\" fails to validate",
                      dimension->name, value);
                return;
             }
          }
       }

    }

    gctx->config = cfg;
    gctx->global_lock_aquire = geocache_context_seeding_lock_aquire;
    gctx->global_lock_release = geocache_context_seeding_lock_release;
    gctx->log = geocache_context_seeding_log;
    ctx->get_next_tile = geocache_context_seeding_get_next_tile;
    ctx->minzoom = minzoom;
    ctx->maxzoom = maxzoom;
    ctx->tileset = tileset;
    ctx->grid_link = grid_link;
    ctx->dimensions = dimensions;
}

void dummy_lock_aquire(geocache_context *ctx){

}

void dummy_lock_release(geocache_context *ctx) {

}

void dummy_log(geocache_context *ctx, geocache_log_level level, char *msg, ...) {

}

static void* APR_THREAD_FUNC doseed(apr_thread_t *thread, void *data) {
    geocache_context_seeding *ctx = (geocache_context_seeding*)data;
    geocache_context *gctx = (geocache_context*)ctx;
    geocache_tile *tile = geocache_tileset_tile_create(gctx->pool, ctx->tileset, ctx->grid_link);
    geocache_context tile_ctx;
    geocache_context_init(&tile_ctx);
    tile_ctx.global_lock_aquire = dummy_lock_aquire;
    tile_ctx.global_lock_release = dummy_lock_release;
    tile_ctx.log = geocache_context_seeding_log;
    tile_ctx.config = gctx->config;
    apr_pool_create(&tile_ctx.pool,NULL);
    while(GEOCACHE_SUCCESS == ctx->get_next_tile(ctx,tile,&tile_ctx) && !sig_int_received) {
        geocache_tileset_tile_get(&tile_ctx,tile);
        if(tile_ctx.get_error(&tile_ctx)) {
            gctx->set_error(gctx,tile_ctx.get_error(&tile_ctx),tile_ctx.get_error_message(&tile_ctx));
            apr_thread_exit(thread,GEOCACHE_FAILURE);
        }
        apr_pool_clear(tile_ctx.pool);
        gctx->global_lock_aquire(gctx);

        seededtiles+=tile->tileset->metasize_x*tile->tileset->metasize_y;
        seededtilestot+=tile->tileset->metasize_x*tile->tileset->metasize_y;
        time_t now = time(NULL);
        if(now-lastlogtime>5) {
           printf("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\rseeding level %d at %.2f tiles/sec (avg:%.2f)\r",tile->z,
                 seededtiles/((double)(now-lastlogtime)),
                 seededtilestot/((double)(now-starttime)));
           fflush(NULL);

           lastlogtime = now;
           seededtiles = 0;
        }
        gctx->global_lock_release(gctx);
    }

    if(gctx->get_error(gctx)) {
        apr_thread_exit(thread,GEOCACHE_FAILURE);
    }
    
    apr_thread_exit(thread,GEOCACHE_SUCCESS);
    return NULL;
}


int usage(const char *progname, char *msg) {
   int i=0;
   if(msg)
      printf("%s\nusage: %s options\n",msg,progname);
   else
      printf("usage: %s options\n",progname);

   while(seed_options[i].name) {
      if(seed_options[i].has_arg==TRUE) {
         printf("-%c|--%s [value]: %s\n",seed_options[i].optch,seed_options[i].name, seed_options[i].description);
      } else {
         printf("-%c|--%s: %s\n",seed_options[i].optch,seed_options[i].name, seed_options[i].description);
      }
      i++;
   }
   return 1;
}

int main(int argc, const char **argv) {
    /* initialize apr_getopt_t */
    apr_getopt_t *opt;
    const char *configfile=NULL;
    geocache_cfg *cfg = NULL;
    geocache_tileset *tileset;
    geocache_context_seeding ctx;
    geocache_context *gctx = (geocache_context*)&ctx;
    apr_thread_t **threads;
    apr_threadattr_t *thread_attrs;
    const char *tileset_name=NULL;
    const char *grid_name = NULL;
    int *zooms = NULL;//[2];
    double *extent = NULL;//[4];
    int nthreads=1;
    int optch;
    int rv,n;
    const char *old = NULL;
    const char *optarg;

#ifdef USE_OGR
    const char *ogr_where = NULL;
    const char *ogr_layer = NULL;
    const char *ogr_sql = NULL;
    const char *ogr_datasource = NULL;
#endif

    apr_initialize();
    (void) signal(SIGINT,handle_sig_int);
    apr_pool_create(&gctx->pool,NULL);
    geocache_context_init(gctx);
    cfg = geocache_configuration_create(gctx->pool);
    gctx->config = cfg;
    gctx->log= geocache_context_seeding_log;
    apr_getopt_init(&opt, gctx->pool, argc, argv);
    
    curz=-1;
    seededtiles=seededtilestot=0;
    starttime = lastlogtime = time(NULL);
    apr_table_t *dimensions = apr_table_make(gctx->pool,3);
    char *dimkey = NULL,*dimvalue = NULL,*key,*last,*optargcpy = NULL;
    int keyidx;

    /* parse the all options based on opt_option[] */
    while ((rv = apr_getopt_long(opt, seed_options, &optch, &optarg)) == APR_SUCCESS) {
        switch (optch) {
            case 'h':
                return usage(argv[0],NULL);
                break;
            case 'c':
                configfile = optarg;
                break;
            case 'g':
                grid_name = optarg;
                break;
            case 't':
                tileset_name = optarg;
                break;
            case 'n':
                nthreads = (int)strtol(optarg, NULL, 10);
                break;
            case 'e':
                if ( GEOCACHE_SUCCESS != geocache_util_extract_double_list(gctx, (char*)optarg, ',', &extent, &n) ||
                        n != 4 || extent[0] >= extent[2] || extent[1] >= extent[3] ) {
                    return usage(argv[0], "failed to parse extent, expecting comma separated 4 doubles");
                }
                break;
            case 'z':
                if ( GEOCACHE_SUCCESS != geocache_util_extract_int_list(gctx, (char*)optarg, ',', &zooms, &n) ||
                        n != 2 || zooms[0] > zooms[1]) {
                    return usage(argv[0], "failed to parse zooms, expecting comma separated 2 ints");
                }
                break;
            case 'o':
                old = optarg;
                break;
            case 'D':
                optargcpy = apr_pstrdup(gctx->pool,optarg);
                keyidx = 0;
                for (key = apr_strtok(optargcpy, "=", &last); key != NULL;
                      key = apr_strtok(NULL, "=", &last)) {
                   if(keyidx == 0) {
                      dimkey = key;
                   } else {
                      dimvalue = key;
                   }
                   keyidx++;
                }
                if(keyidx!=2 || !dimkey || !dimvalue || !*dimkey || !*dimvalue) {
                   return usage(argv[0], "failed to parse dimension, expecting DIMNAME=DIMVALUE");
                }
                apr_table_set(dimensions,dimkey,dimvalue);
                break;
#ifdef USE_OGR
            case 'd':
                ogr_datasource = optarg;
                break;
            case 's':
                ogr_sql = optarg;
                break;
            case 'l':
                ogr_layer = optarg;
                break;
            case 'w':
               ogr_where = optarg;
               break;
#endif

        }
    }
    if (rv != APR_EOF) {
        return usage(argv[0],"bad options");
    }

    if( ! configfile ) {
        return usage(argv[0],"config not specified");
    } else {
        geocache_configuration_parse(gctx,configfile,cfg);
        if(gctx->get_error(gctx))
            return usage(argv[0],gctx->get_error_message(gctx));
    }

#ifdef USE_OGR
    if(extent && ogr_datasource) {
       return usage(argv[0], "cannot specify both extent and ogr-datasource");
    }

    if( ogr_sql && ( ogr_where || ogr_layer )) {
      return usage(argv[0], "ogr-where or ogr_layer cannot be used in conjunction with ogr-sql");
    }

    if(ogr_datasource) {
       OGRRegisterAll();
       OGRDataSourceH hDS = NULL;
       OGRLayerH layer = NULL;
       hDS = OGROpen( ogr_datasource, FALSE, NULL );
       if( hDS == NULL )
       {
          printf( "OGR Open failed\n" );
          exit( 1 );
       }

       if(ogr_sql) {
         layer = OGR_DS_ExecuteSQL( hDS, ogr_sql, NULL, NULL);
         if(!layer) {
            return usage(argv[0],"aborting");
         }
       } else {
         int nLayers = OGR_DS_GetLayerCount(hDS);
         if(nLayers>1 && !ogr_layer) {
            return usage(argv[0],"ogr datastore contains more than one layer. please specify which one to use with --ogr-layer");
         } else {
            if(ogr_layer) {
               layer = OGR_DS_GetLayerByName(hDS,ogr_layer);
            } else {
               layer = OGR_DS_GetLayer(hDS,0);
            }
            if(!layer) {
               return usage(argv[0],"aborting");
            }
            if(ogr_where) {
               if(OGRERR_NONE != OGR_L_SetAttributeFilter(layer, ogr_where)) {
                  return usage(argv[0],"aborting");
               }
            }

         }
       }
      if((nClippers=OGR_L_GetFeatureCount(layer, TRUE)) == 0) {
         return usage(argv[0],"no features in provided ogr parameters, cannot continue");
      }


      clippers = (OGRGeometryH*)malloc(nClippers*sizeof(OGRGeometryH));


      OGRFeatureH hFeature;

      OGR_L_ResetReading(layer);
      extent = apr_pcalloc(gctx->pool,4*sizeof(double));
      int f=0;
      while( (hFeature = OGR_L_GetNextFeature(layer)) != NULL ) {
         clippers[f] = OGR_G_Clone(OGR_F_GetGeometryRef(hFeature));
         OGREnvelope ogr_extent;
         OGR_G_GetEnvelope	(clippers[f], &ogr_extent);	
         if(f == 0) {
            extent[0] = ogr_extent.MinX;
            extent[1] = ogr_extent.MinY;
            extent[2] = ogr_extent.MaxX;
            extent[3] = ogr_extent.MaxY;
         } else {
            extent[0] = GEOCACHE_MIN(ogr_extent.MinX, extent[0]);
            extent[1] = GEOCACHE_MIN(ogr_extent.MinY, extent[1]);
            extent[2] = GEOCACHE_MAX(ogr_extent.MaxX, extent[2]);
            extent[3] = GEOCACHE_MAX(ogr_extent.MaxY, extent[3]);
         }

         OGR_F_Destroy( hFeature );
         f++;
      }
      if(f != nClippers) {
         usage(argv[0],"error");
      }
      

    }
#endif

    geocache_grid_link *grid_link = NULL;

    if( ! tileset_name ) {
        return usage(argv[0],"tileset not specified");
    } else {
        tileset = geocache_configuration_get_tileset(cfg,tileset_name);
        if(!tileset) {
            return usage(argv[0], "tileset not found in configuration");
        }
        if( ! grid_name ) {
           grid_link = APR_ARRAY_IDX(tileset->grid_links,0,geocache_grid_link*);
        } else {
           int i;
           for(i=0;i<tileset->grid_links->nelts;i++) {
              geocache_grid_link *sgrid = APR_ARRAY_IDX(tileset->grid_links,i,geocache_grid_link*);
              if(!strcmp(sgrid->grid->name,grid_name)) {
               grid_link = sgrid;
               break;
              }
           }
           if(!grid_link) {
              return usage(argv[0],"grid not configured for tileset");
           }
        }
        if(!zooms) {
            zooms = (int*)apr_pcalloc(gctx->pool,2*sizeof(int));
            zooms[0] = 0;
            zooms[1] = grid_link->grid->nlevels - 1;
        }
        if(zooms[0]<0) zooms[0] = 0;
        if(zooms[1]>= grid_link->grid->nlevels) zooms[1] = grid_link->grid->nlevels - 1;
    }

    if(old) {
      struct tm oldtime;
      memset(&oldtime,0,sizeof(oldtime));
      char *ret = strptime(old,"%Y/%m/%d %H:%M",&oldtime);
      if(!ret || *ret){
         return usage(argv[0],"failed to parse time");
      }
      if(APR_SUCCESS != apr_time_ansi_put(&age_limit,mktime(&oldtime))) {
         return usage(argv[0],"failed to convert time");
      }
    }

    geocache_context_seeding_init(&ctx,cfg,tileset,zooms[0],zooms[1],grid_link, dimensions);
    if(gctx->get_error(gctx)) {
        printf("%s",gctx->get_error_message(gctx));
        return 1;
    }
    if(extent) {
       // update the grid limits
       geocache_grid_compute_limits(grid_link->grid,extent,grid_link->grid_limits);
    }

    /* adjust our grid limits so they align on the metatile limits
     * we need to do this because the seeder does not check for individual tiles, it
     * goes from one metatile to the next*/

    for(n=0;n<grid_link->grid->nlevels;n++) {
      grid_link->grid_limits[n][0] = (grid_link->grid_limits[n][0]/tileset->metasize_x)*tileset->metasize_x;
      grid_link->grid_limits[n][1] = (grid_link->grid_limits[n][1]/tileset->metasize_y)*tileset->metasize_y;
      grid_link->grid_limits[n][2] = (grid_link->grid_limits[n][2]/tileset->metasize_x+1)*tileset->metasize_x;
      grid_link->grid_limits[n][3] = (grid_link->grid_limits[n][3]/tileset->metasize_y+1)*tileset->metasize_y;
    }

    ctx.nextz = zooms[0];
    ctx.nextx = grid_link->grid_limits[zooms[0]][0];
    ctx.nexty = grid_link->grid_limits[zooms[0]][1];

    /* find the first tile to render if the first one already exists */
    if(!should_seed_tile(&ctx, ctx.nextx, ctx.nexty, ctx.nextz, gctx)) {
       geocache_tile *tile = geocache_tileset_tile_create(gctx->pool, tileset, grid_link);
       geocache_context_seeding_get_next_tile(&ctx,tile,gctx);
       if(ctx.nextz > ctx.maxzoom) {
          printf("nothing to do, all tiles are present\n");
          return 0;
       }

    } 


    if( ! nthreads ) {
        return usage(argv[0],"failed to parse nthreads, must be int");
    } else {
        apr_threadattr_create(&thread_attrs, gctx->pool);
        threads = (apr_thread_t**)apr_pcalloc(gctx->pool, nthreads*sizeof(apr_thread_t*));
        for(n=0;n<nthreads;n++) {
            apr_thread_create(&threads[n], thread_attrs, doseed, (void*)&ctx, gctx->pool);
        }
        for(n=0;n<nthreads;n++) {
            apr_thread_join(&rv, threads[n]);
        }
    }
    if(gctx->get_error(gctx)) {
        printf("%s",gctx->get_error_message(gctx));
    }

    if(seededtilestot>0)
      printf("seeded %d tiles at %g tiles/sec\n",seededtilestot, seededtilestot/((double)(time(NULL)-starttime)));

    return 0;
}
/* vim: ai ts=3 sts=3 et sw=3
*/

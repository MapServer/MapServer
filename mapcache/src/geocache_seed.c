#include "geocache.h"
#include <apr_thread_proc.h>
#include <apr_thread_mutex.h>
#include <apr_getopt.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <apr_time.h>
#include <apr_queue.h>
#include <apr_strings.h>

#if defined(USE_OGR) && defined(USE_GEOS)
#define USE_CLIPPERS
#endif

#ifdef USE_CLIPPERS
#include "ogr_api.h"
#include "geos_c.h"
int nClippers = 0;
const GEOSPreparedGeometry **clippers=NULL;
#endif

geocache_tileset *tileset;
geocache_cfg *cfg;
geocache_context ctx;
apr_table_t *dimensions;
int minzoom=0;
int maxzoom=0;
geocache_grid_link *grid_link;
int nthreads=1;
int quiet = 0;
int sig_int_received = 0;
int error_detected = 0;
apr_queue_t *work_queue;

apr_time_t age_limit = 0;
int seededtilestot=0, seededtiles=0, queuedtilestot=0;
struct timeval lastlogtime,starttime;

typedef enum {
   GEOCACHE_CMD_SEED,
   GEOCACHE_CMD_STOP,
   GEOCACHE_CMD_DELETE,
   GEOCACHE_CMD_SKIP
} cmd;

struct seed_cmd {
   cmd command;
   int x;
   int y;
   int z;
};

cmd mode = GEOCACHE_CMD_SEED; /* the mode the utility will be running in: either seed or delete */

static const apr_getopt_option_t seed_options[] = {
    /* long-option, short-option, has-arg flag, description */
    { "config", 'c', TRUE, "configuration file (/path/to/geocacahe.xml)"},
    { "tileset", 't', TRUE, "tileset to seed" },
    { "grid", 'g', TRUE, "grid to seed" },
    { "zoom", 'z', TRUE, "min and max zoomlevels to seed, separated by a comma. eg 0,6" },
    { "extent", 'e', TRUE, "extent to seed, format: minx,miny,maxx,maxy" },
    { "nthreads", 'n', TRUE, "number of parallel threads to use" },
    { "mode", 'm', TRUE, "mode: seed (default) or delete" },
    { "older", 'o', TRUE, "reseed tiles older than supplied date (format: year/month/day hour:minute, eg: 2011/01/31 20:45" },
    { "dimension", 'D', TRUE, "set the value of a dimension (format DIMENSIONNAME=VALUE). Can be used multiple times for multiple dimensions" },
#ifdef USE_CLIPPERS
    { "ogr-datasource", 'd', TRUE, "ogr datasource to get features from"},
    { "ogr-layer", 'l', TRUE, "layer inside datasource"},
    { "ogr-sql", 's', TRUE, "sql to filter inside layer"},
    { "ogr-where", 'w', TRUE, "filter to apply on layer features"},
#endif
    { "help", 'h', FALSE, "show help" },
    { "quiet", 'q', FALSE, "don't show progress info" },
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

void dummy_lock_aquire(geocache_context *ctx){}
void dummy_lock_release(geocache_context *ctx){}
void dummy_log(geocache_context *ctx, geocache_log_level level, char *msg, ...){}


void geocache_context_seeding_log(geocache_context *ctx, geocache_log_level level, char *msg, ...) {
    va_list args;
    va_start(args,msg);
    vfprintf(stderr,msg,args);
    va_end(args);
}

#ifdef USE_CLIPPERS
int ogr_features_intersect_tile(geocache_context *ctx, geocache_tile *tile) {
   geocache_metatile *mt = geocache_tileset_metatile_get(ctx,tile);
   GEOSCoordSequence *mtbboxls = GEOSCoordSeq_create(5,2);
   double *e = mt->map.extent;
   GEOSCoordSeq_setX(mtbboxls,0,e[0]);
   GEOSCoordSeq_setY(mtbboxls,0,e[1]);
   GEOSCoordSeq_setX(mtbboxls,1,e[2]);
   GEOSCoordSeq_setY(mtbboxls,1,e[1]);
   GEOSCoordSeq_setX(mtbboxls,2,e[2]);
   GEOSCoordSeq_setY(mtbboxls,2,e[3]);
   GEOSCoordSeq_setX(mtbboxls,3,e[0]);
   GEOSCoordSeq_setY(mtbboxls,3,e[3]);
   GEOSCoordSeq_setX(mtbboxls,4,e[0]);
   GEOSCoordSeq_setY(mtbboxls,4,e[1]);
   GEOSGeometry *mtbbox = GEOSGeom_createLinearRing(mtbboxls);
   GEOSGeometry *mtbboxg = GEOSGeom_createPolygon(mtbbox,NULL,0);
   int i;
   int intersects = 0;
   for(i=0;i<nClippers;i++) {
      const GEOSPreparedGeometry *clipper = clippers[i];
      if(GEOSPreparedIntersects(clipper,mtbboxg)) {
         intersects = 1;
         break;
      }
   }
   GEOSGeom_destroy(mtbboxg);
   return intersects;
}

#endif

int lastmsglen = 0;
void progresslog(int x, int y, int z) {
   if(quiet) return;
   char msg[1024];
   if(queuedtilestot>nthreads) {
      seededtilestot = queuedtilestot - nthreads;
      struct timeval now_t;
      gettimeofday(&now_t,NULL);
      float duration = ((now_t.tv_sec-lastlogtime.tv_sec)*1000000+(now_t.tv_usec-lastlogtime.tv_usec))/1000000.0;
      float totalduration = ((now_t.tv_sec-starttime.tv_sec)*1000000+(now_t.tv_usec-starttime.tv_usec))/1000000.0;
      if(duration>=5) {
         int Nx, Ny;
         Nx = (grid_link->grid_limits[z][2]-grid_link->grid_limits[z][0])/tileset->metasize_x;
         Ny = (grid_link->grid_limits[z][3]-grid_link->grid_limits[z][1])/tileset->metasize_y;
         int Ntot = Nx*Ny;
         int Ncur = (y-grid_link->grid_limits[z][1])/tileset->metasize_y*Nx+(x-grid_link->grid_limits[z][0]+1)/tileset->metasize_x;
         int ntilessincelast = seededtilestot-seededtiles;
         sprintf(msg,"seeding level %d [%d/%d]: %f metatiles/sec (avg since start: %f)",z,Ncur,Ntot,ntilessincelast/duration,
               seededtilestot/totalduration);
         lastlogtime=now_t;
         seededtiles=seededtilestot;
      } else {
         return;
      }
   } else {
      sprintf(msg,"seeding level %d",z);
   
   }
   if(lastmsglen) {
      char erasestring[1024];
      int len = GEOCACHE_MIN(1023,lastmsglen);
      memset(erasestring,' ',len);
      erasestring[len+1]='\0';
      sprintf(erasestring,"\r%%%ds\r",lastmsglen);
      printf(erasestring," ");
   }
   lastmsglen = strlen(msg);
   printf("%s",msg);
   fflush(NULL);
}



void cmd_thread() {
   int z = minzoom;
   int x = grid_link->grid_limits[z][0];
   int y = grid_link->grid_limits[z][1];
   geocache_context cmd_ctx = ctx;
   apr_pool_create(&cmd_ctx.pool,ctx.pool);
   geocache_tile *tile = geocache_tileset_tile_create(ctx.pool, tileset, grid_link);
   tile->dimensions = dimensions;

   while(1) {
      apr_pool_clear(cmd_ctx.pool);
      if(sig_int_received || error_detected) { //stop if we were asked to stop by hitting ctrl-c
         //remove all items from the queue
         void *entry;
         while (apr_queue_trypop(work_queue,&entry)!=APR_EAGAIN) {queuedtilestot--;}
         break;
      }
      cmd action = GEOCACHE_CMD_SKIP;
      tile->x = x;
      tile->y = y;
      tile->z = z;
      int tile_exists = tileset->cache->tile_exists(&cmd_ctx,tile);
      int intersects = -1;
      /* if the tile exists and a time limit was specified, check the tile modification date */
      if(tile_exists) {
         if(age_limit) {
            if(tileset->cache->tile_get(&cmd_ctx,tile) == GEOCACHE_SUCCESS) {
               if(tile->mtime && tile->mtime<age_limit) {
                  /* the tile modification time is older than the specified limit */
#ifdef USE_CLIPPERS
                  /* check we are in the requested features before deleting the tile */
                  if(nClippers > 0) {
                     intersects = ogr_features_intersect_tile(&cmd_ctx,tile);
                  }
#endif
                  if(intersects != 0) {
                     /* the tile intersects the ogr features, or there was no clipping asked for: seed it */
                     if(mode == GEOCACHE_CMD_SEED) {
                        geocache_tileset_tile_delete(&cmd_ctx,tile,GEOCACHE_TRUE);
                        action = GEOCACHE_CMD_SEED;
                     }
                     else { //if(action == GEOCACHE_CMD_DELETE)
                        action = GEOCACHE_CMD_DELETE;
                     }
                  } else {
                     /* the tile does not intersect the ogr features, and already exists, do nothing */
                     action = GEOCACHE_CMD_SKIP;
                  }
               }
            } else {
               //BUG: tile_exists returned true, but tile_get returned a failure. not sure what to do.
               action = GEOCACHE_CMD_SKIP;
            }
         } else {
            if(mode == GEOCACHE_CMD_DELETE) {
               //the tile exists and we are in delete mode: delete it
               action = GEOCACHE_CMD_DELETE;
            } else {
               // the tile exists and we are in seed mode, skip to next one
               action = GEOCACHE_CMD_SKIP;
            }
         }
      } else {
         // the tile does not exist
         if(mode == GEOCACHE_CMD_SEED) {
#ifdef USE_CLIPPERS
         /* check we are in the requested features before deleting the tile */
         if(nClippers > 0) {
            if(ogr_features_intersect_tile(&cmd_ctx,tile)) {
               action = GEOCACHE_CMD_SEED;
            } else {
               action = GEOCACHE_CMD_SKIP;
            }
         } else {
            action = GEOCACHE_CMD_SEED;
         }
#else
         action = GEOCACHE_CMD_SEED;
#endif
         } else {
            action = GEOCACHE_CMD_SKIP;
         }
      }

      if(action == GEOCACHE_CMD_SEED || action == GEOCACHE_CMD_DELETE){
         //current x,y,z needs seeding, add it to the queue
         struct seed_cmd *cmd = malloc(sizeof(struct seed_cmd));
         cmd->x = x;
         cmd->y = y;
         cmd->z = z;
         cmd->command = action;
         apr_queue_push(work_queue,cmd);
         queuedtilestot++;
         progresslog(x,y,z);
      }
      //compute next x,y,z
      x += tileset->metasize_x;
      if(x >= grid_link->grid_limits[z][2]) {
         //x is too big, increment y
         y += tileset->metasize_y;
         if(y >= grid_link->grid_limits[z][3]) {
            //y is too big, increment z
            z += 1;
            if(z > maxzoom) break; //we've finished seeding
            y = grid_link->grid_limits[z][1]; //set y to the smallest value for current z
         }
         x = grid_link->grid_limits[z][0]; //set x to smallest value for current z
      }
   }

   if(error_detected) {
      printf("%s\n",ctx.get_error_message(&ctx));
   }

   //instruct rendering threads to stop working
   int i;
   for(i=0;i<nthreads;i++) {
      struct seed_cmd *cmd = malloc(sizeof(struct seed_cmd));
      cmd->command = GEOCACHE_CMD_STOP;
      apr_queue_push(work_queue,cmd);
   }
}

static void* APR_THREAD_FUNC seed_thread(apr_thread_t *thread, void *data) {
   geocache_context seed_ctx = ctx;
   seed_ctx.log = dummy_log;
   apr_pool_create(&seed_ctx.pool,ctx.pool);
   geocache_tile *tile = geocache_tileset_tile_create(ctx.pool, tileset, grid_link);
   tile->dimensions = dimensions;
   while(1) {
      apr_pool_clear(seed_ctx.pool);
      struct seed_cmd *cmd;
      apr_queue_pop(work_queue, (void**)&cmd);
      if(cmd->command == GEOCACHE_CMD_STOP) break;
      tile->x = cmd->x;
      tile->y = cmd->y;
      tile->z = cmd->z;
      if(cmd->command == GEOCACHE_CMD_SEED) {
         geocache_tileset_tile_get(&seed_ctx,tile);
      } else { //CMD_DELETE
         geocache_tileset_tile_delete(&seed_ctx,tile,GEOCACHE_TRUE);
      }
      if(seed_ctx.get_error(&seed_ctx)) {
         error_detected++;
         ctx.log(&ctx,GEOCACHE_INFO,seed_ctx.get_error_message(&seed_ctx));
      }
      free(cmd);
   }
   apr_thread_exit(thread,GEOCACHE_SUCCESS);
   return NULL;
}


void
notice(const char *fmt, ...) {
        va_list ap;

        fprintf( stdout, "NOTICE: ");
       
        va_start (ap, fmt);
        vfprintf( stdout, fmt, ap);
        va_end(ap);
        fprintf( stdout, "\n" );
}

void
log_and_exit(const char *fmt, ...) {
        va_list ap;

        fprintf( stdout, "ERROR: ");
       
        va_start (ap, fmt);
        vfprintf( stdout, fmt, ap);
        va_end(ap);
        fprintf( stdout, "\n" );
        exit(1);
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
    apr_thread_t **threads;
    apr_threadattr_t *thread_attrs;
    const char *tileset_name=NULL;
    const char *grid_name = NULL;
    int *zooms = NULL;//[2];
    double *extent = NULL;//[4];
    int optch;
    int rv,n;
    const char *old = NULL;
    const char *optarg;

#ifdef USE_CLIPPERS
    const char *ogr_where = NULL;
    const char *ogr_layer = NULL;
    const char *ogr_sql = NULL;
    const char *ogr_datasource = NULL;
#endif

    apr_initialize();
    (void) signal(SIGINT,handle_sig_int);
    apr_pool_create(&ctx.pool,NULL);
    geocache_context_init(&ctx);
    cfg = geocache_configuration_create(ctx.pool);
    ctx.config = cfg;
    ctx.log= geocache_context_seeding_log;
    ctx.global_lock_aquire = dummy_lock_aquire;
    ctx.global_lock_release = dummy_lock_release;
    apr_getopt_init(&opt, ctx.pool, argc, argv);
    
    seededtiles=seededtilestot=queuedtilestot=0;
    gettimeofday(&starttime,NULL);
    lastlogtime=starttime;
    apr_table_t *argdimensions = apr_table_make(ctx.pool,3);
    char *dimkey = NULL,*dimvalue = NULL,*key,*last,*optargcpy = NULL;
    int keyidx;

    /* parse the all options based on opt_option[] */
    while ((rv = apr_getopt_long(opt, seed_options, &optch, &optarg)) == APR_SUCCESS) {
        switch (optch) {
            case 'h':
                return usage(argv[0],NULL);
                break;
            case 'q':
                quiet = 1;
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
            case 'm':
                if(!strcmp(optarg,"delete")) {
                   mode = GEOCACHE_CMD_DELETE;
                } else if(strcmp(optarg,"seed")){
                   return usage(argv[0],"invalid mode, expecting \"seed\" or \"delete\"");
                } else {
                   mode = GEOCACHE_CMD_SEED;
                }
                break;
            case 'n':
                nthreads = (int)strtol(optarg, NULL, 10);
                break;
            case 'e':
                if ( GEOCACHE_SUCCESS != geocache_util_extract_double_list(&ctx, (char*)optarg, ',', &extent, &n) ||
                        n != 4 || extent[0] >= extent[2] || extent[1] >= extent[3] ) {
                    return usage(argv[0], "failed to parse extent, expecting comma separated 4 doubles");
                }
                break;
            case 'z':
                if ( GEOCACHE_SUCCESS != geocache_util_extract_int_list(&ctx, (char*)optarg, ',', &zooms, &n) ||
                        n != 2 || zooms[0] > zooms[1]) {
                    return usage(argv[0], "failed to parse zooms, expecting comma separated 2 ints");
                } else {
                   minzoom = zooms[0];
                   maxzoom = zooms[1];
                }
                break;
            case 'o':
                old = optarg;
                break;
            case 'D':
                optargcpy = apr_pstrdup(ctx.pool,optarg);
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
                apr_table_set(argdimensions,dimkey,dimvalue);
                break;
#ifdef USE_CLIPPERS
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
        geocache_configuration_parse(&ctx,configfile,cfg);
        if(ctx.get_error(&ctx))
            return usage(argv[0],ctx.get_error_message(&ctx));
        geocache_configuration_post_config(&ctx,cfg);
        if(ctx.get_error(&ctx))
            return usage(argv[0],ctx.get_error_message(&ctx));
    }

#ifdef USE_CLIPPERS
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


      initGEOS(notice, log_and_exit);
      clippers = (const GEOSPreparedGeometry**)malloc(nClippers*sizeof(GEOSPreparedGeometry*));


      OGRFeatureH hFeature;
      GEOSWKTReader *geoswktreader = GEOSWKTReader_create();
      OGR_L_ResetReading(layer);
      extent = apr_pcalloc(ctx.pool,4*sizeof(double));
      int f=0;
      while( (hFeature = OGR_L_GetNextFeature(layer)) != NULL ) {
         OGRGeometryH geom = OGR_F_GetGeometryRef(hFeature);
         if(!geom ||  !OGR_G_IsValid(geom)) continue;
         char *wkt;
         OGR_G_ExportToWkt(geom,&wkt);
         GEOSGeometry *geosgeom = GEOSWKTReader_read(geoswktreader,wkt);
         free(wkt);
         clippers[f] = GEOSPrepare(geosgeom);
         //GEOSGeom_destroy(geosgeom);
         OGREnvelope ogr_extent;
         OGR_G_GetEnvelope	(geom, &ogr_extent);	
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
      nClippers = f;
      

    }
#endif

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
        if(minzoom == 0 && maxzoom ==0) {
            minzoom = 0;
            maxzoom = grid_link->grid->nlevels - 1;
        }
        if(minzoom<0) minzoom = 0;
        if(maxzoom>= grid_link->grid->nlevels) maxzoom = grid_link->grid->nlevels - 1;
    }


    if(old) {
       if(strcasecmp(old,"now")) {
          struct tm oldtime;
          memset(&oldtime,0,sizeof(oldtime));
          char *ret = strptime(old,"%Y/%m/%d %H:%M",&oldtime);
          if(!ret || *ret){
             return usage(argv[0],"failed to parse time");
          }
          if(APR_SUCCESS != apr_time_ansi_put(&age_limit,mktime(&oldtime))) {
             return usage(argv[0],"failed to convert time");
          }
       } else {
         age_limit = apr_time_now();
       }
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
    
    /* validate the supplied dimensions */
    if (!apr_is_empty_array(tileset->dimensions)) {

       dimensions = apr_table_make(ctx.pool,3);
       int i;
       for(i=0;i<tileset->dimensions->nelts;i++) {
          geocache_dimension *dimension = APR_ARRAY_IDX(tileset->dimensions,i,geocache_dimension*);
          const char *value;
          if((value = (char*)apr_table_get(argdimensions,dimension->name)) != NULL) {
             char *tmpval = apr_pstrdup(ctx.pool,value);
             int ok = dimension->validate(&ctx,dimension,&tmpval);
             if(GC_HAS_ERROR(&ctx) || ok != GEOCACHE_SUCCESS ) {
                return usage(argv[0],"failed to validate dimension");
               return 1;
             } else {
                /* validate may have changed the dimension value, so set that value into the dimensions table */
                apr_table_setn(dimensions,dimension->name,tmpval);
             }
          } else {
             /* a dimension was not specified on the command line, add the default value */
             apr_table_setn(dimensions, dimension->name, dimension->default_value);
          }
       }

    }

    if( ! nthreads ) {
        return usage(argv[0],"failed to parse nthreads, must be int");
    } else {
        //start the thread that will populate the queue.
        //create the queue where tile requests will be put
        apr_queue_create(&work_queue,nthreads,ctx.pool);

        //start the rendering threads.
        apr_threadattr_create(&thread_attrs, ctx.pool);
        threads = (apr_thread_t**)apr_pcalloc(ctx.pool, nthreads*sizeof(apr_thread_t*));
        for(n=0;n<nthreads;n++) {
           apr_thread_create(&threads[n], thread_attrs, seed_thread, NULL, ctx.pool);
        }
        cmd_thread();
        for(n=0;n<nthreads;n++) {
           apr_thread_join(&rv, threads[n]);
        }
        if(ctx.get_error(&ctx)) {
           printf("%s",ctx.get_error_message(&ctx));
        }

        if(seededtilestot>0) {
           struct timeval now_t;
           gettimeofday(&now_t,NULL);
           float duration = ((now_t.tv_sec-starttime.tv_sec)*1000000+(now_t.tv_usec-starttime.tv_usec))/1000000.0;
           printf("\nseeded %d metatiles at %g tiles/sec\n",seededtilestot, seededtilestot/duration);
        }
    }
    return 0;
}
/* vim: ai ts=3 sts=3 et sw=3
*/

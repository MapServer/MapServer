#include "geocache.h"
#include <apr_thread_proc.h>
#include <apr_getopt.h>

static const apr_getopt_option_t seed_options[] = {
    /* long-option, short-option, has-arg flag, description */
    { "config", 'c', TRUE, "configuration file"},
    { "tileset", 't', TRUE, "tileset to seed" },
    { "zoom", 'z', TRUE, "min and max zoomlevels to seed" },
    { "extent", 'e', TRUE, "extent" },
    { "nthreads", 'n', TRUE, "number of parallel threads to use" },
    { "help", 'h', FALSE, "show help" },    
    { NULL, 0, 0, NULL },
};

static void* APR_THREAD_FUNC doseed(apr_thread_t *thd, void *data) {
    int thread_id = (int)data;
    int i;
    for(i=0;i<10;i++) {
        printf("thread %d: %d\n",thread_id,i);
    }
    apr_thread_exit(thd,APR_SUCCESS);
    return NULL;
}


int usage(const char *progname, char *msg) {
    printf("%s\nusage: %s options\n"
            "-c|--config conffile : configuration file to load\n"
            "-t|--tileset tileset : name of the tileset to seed\n"
            "[-z|--zoom minzoom,maxzoom] : zoomlevels to seed\n"
            "[-e|--extent minx,miny,maxx,maxy] : extent to seed\n"
            "[-n|--nthreads n] : number of parallel threads\n",
            msg,progname);
    return 1;
}

int main(int argc, const char **argv) {
    /* initialize apr_getopt_t */
    apr_getopt_t *opt;
    const char *configfile=NULL;
    geocache_cfg *cfg = NULL;
    geocache_tileset *tileset;
    geocache_context ctx;
    apr_thread_t **threads;
    apr_threadattr_t *thread_attrs;
    const char *tileset_name=NULL;
    int *zooms = NULL;//[2];
    double *extent = NULL;//[4];
    int nthreads=1;
    int optch;
    int rv,n;
    const char *optarg;
    apr_initialize();
    apr_pool_create_core(&ctx.pool);
    geocache_context_init(&ctx);
    cfg = geocache_configuration_create(ctx.pool);
    apr_getopt_init(&opt, ctx.pool, argc, argv);
    /* parse the all options based on opt_option[] */
    while ((rv = apr_getopt_long(opt, seed_options, &optch, &optarg)) == APR_SUCCESS) {
        switch (optch) {
            case 'h':
                return usage(argv[0],NULL);
                break;
            case 'c':
                configfile = optarg;
                break;
            case 't':
                tileset_name = optarg;
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
                        n != 2 || zooms[0] >= zooms[1]) {
                    return usage(argv[0], "failed to parse zooms, expecting comma separated 2 ints");
                }
                break;
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
    }

    if( ! tileset_name ) {
        return usage(argv[0],"tileset not specified");
    } else {
        tileset = geocache_configuration_get_tileset(cfg,tileset_name);
        if(!tileset) {
            return usage(argv[0], "tileset not found in configuration");
        }
    }

    if( ! nthreads ) {
        return usage(argv[0],"failed to parse nthreads, must be int");
    } else {
        apr_threadattr_create(&thread_attrs, ctx.pool);
        threads = (apr_thread_t**)apr_pcalloc(ctx.pool, nthreads*sizeof(apr_thread_t*));
        for(n=0;n<nthreads;n++) {
            apr_thread_create(&threads[n], thread_attrs, doseed, (void*)n, ctx.pool);
        }
        for(n=0;n<nthreads;n++) {
            apr_thread_join(&rv, threads[n]);
        }

    }


    return 0;
}

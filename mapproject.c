/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  projectionObj / PROJ.4 interface.
 * Author:   Steve Lime and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2005 Regents of the University of Minnesota.
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
 ****************************************************************************/

#include "mapserver.h"
#include "mapproject.h"
#include "mapthread.h"
#include <assert.h>
#include <float.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "mapaxisorder.h"

#include "cpl_conv.h"
#include "ogr_srs_api.h"

static char *ms_proj_lib = NULL;
#if PROJ_VERSION_MAJOR >= 6
static unsigned ms_proj_lib_change_counter = 0;
#endif

typedef struct LinkedListOfProjContext LinkedListOfProjContext;
struct LinkedListOfProjContext
{
    LinkedListOfProjContext* next;
    projectionContext* context;
};

static LinkedListOfProjContext* headOfLinkedListOfProjContext = NULL;

static int msTestNeedWrap( pointObj pt1, pointObj pt2, pointObj pt2_geo,
                           reprojectionObj* reprojector );


static projectionContext* msProjectionContextCreate(void);
static void msProjectionContextUnref(projectionContext* ctx);

#if PROJ_VERSION_MAJOR >= 6

#include "proj_experimental.h"

struct reprojectionObj
{
    projectionObj* in;
    projectionObj* out;
    PJ* pj;
    int should_do_line_cutting;
    shapeObj splitShape;
    int bFreePJ;
};

/* Helps considerably for use cases like msautotest/wxs/wms_inspire.map */
/* which involve a number of layers with same SRS, and a number of exposed */
/* output SRS */
#define PJ_CACHE_ENTRY_SIZE 32

typedef struct
{
    char* inStr;
    char* outStr;
    PJ* pj;
} pjCacheEntry;

struct projectionContext
{
    PJ_CONTEXT* proj_ctx;
    unsigned ms_proj_lib_change_counter;
    int ref_count;
    pjCacheEntry pj_cache[PJ_CACHE_ENTRY_SIZE];
    int pj_cache_size;
};

/************************************************************************/
/*                  msProjectHasLonWrapOrOver()                         */
/************************************************************************/

static int msProjectHasLonWrapOrOver(projectionObj *in) {
    int i;
    for( i = 0; i < in->numargs; i++ )
    {
        if( strncmp(in->args[i], "lon_wrap=", strlen("lon_wrap=")) == 0 ||
            strcmp(in->args[i], "+over") == 0 ||
            strcmp(in->args[i], "over") == 0 )
        {
            return MS_TRUE;
        }
    }
    return MS_FALSE;
}

static char* getStringFromArgv(int argc, char** args)
{
    int i;
    int len = 0;
    for( i = 0; i < argc; i++ )
    {
        len += strlen(args[i]) + 1;
    }
    char* str = msSmallMalloc(len + 1);
    len = 0;
    for( i = 0; i < argc; i++ )
    {
        size_t arglen = strlen(args[i]);
        memcpy(str + len, args[i], arglen);
        len += arglen;
        str[len] = ' ';
        len ++;
    }
    str[len] = 0;
    return str;
}

/************************************************************************/
/*                         createNormalizedPJ()                         */
/************************************************************************/

/* Return to be freed with proj_destroy() if *pbFreePJ = TRUE */
static PJ* createNormalizedPJ(projectionObj *in, projectionObj *out, int* pbFreePJ)
{
    if( in->proj == out->proj )
    {
        /* Special case to avoid out_str below to cause in_str to become invalid */
        *pbFreePJ = TRUE;
#if PROJ_VERSION_MAJOR == 6 && PROJ_VERSION_MINOR == 0
        /* 6.0 didn't support proj=noop */
        return proj_create(in->proj_ctx->proj_ctx, "+proj=affine");
#else
        return proj_create(in->proj_ctx->proj_ctx, "+proj=noop");
#endif
    }

    const char* const wkt_options[] = { "MULTILINE=NO", NULL };
    const char* in_str = msProjectHasLonWrapOrOver(in) ?
        proj_as_proj_string(in->proj_ctx->proj_ctx, in->proj, PJ_PROJ_4, NULL) :
        proj_as_wkt(in->proj_ctx->proj_ctx, in->proj, PJ_WKT2_2018, wkt_options);
    const char* out_str = msProjectHasLonWrapOrOver(out) ?
        proj_as_proj_string(out->proj_ctx->proj_ctx, out->proj, PJ_PROJ_4, NULL) :
        proj_as_wkt(out->proj_ctx->proj_ctx, out->proj, PJ_WKT2_2018, wkt_options);
    PJ* pj_raw;
    PJ* pj_normalized;
    if( !in_str || !out_str )
        return NULL;
    char* in_str_for_cache = getStringFromArgv(in->numargs, in->args);
    char* out_str_for_cache = getStringFromArgv(out->numargs, out->args);

    if( in->proj_ctx->proj_ctx == out->proj_ctx->proj_ctx )
    {
        int i;
        pjCacheEntry* pj_cache = in->proj_ctx->pj_cache;
        for( i = 0; i < in->proj_ctx->pj_cache_size; i++ )
        {
            if (strcmp(pj_cache[i].inStr, in_str_for_cache) == 0 &&
                strcmp(pj_cache[i].outStr, out_str_for_cache) == 0 )
            {
                PJ* ret = pj_cache[i].pj;
                if( i != 0 )
                {
                    /* Move entry to top */
                    pjCacheEntry tmp;
                    memcpy(&tmp, &pj_cache[i], sizeof(pjCacheEntry));
                    memmove(&pj_cache[1], &pj_cache[0], i * sizeof(pjCacheEntry));
                    memcpy(&pj_cache[0], &tmp, sizeof(pjCacheEntry));
                }
#ifdef notdef
                fprintf(stderr, "cache hit!\n");
#endif
                *pbFreePJ = FALSE;
                msFree(in_str_for_cache);
                msFree(out_str_for_cache);
                return ret;
            }
        }
    }

#ifdef notdef
    fprintf(stderr, "%s -> %s\n", in_str, out_str);
    fprintf(stderr, "%p -> %p\n", in->proj_ctx->proj_ctx, out->proj_ctx->proj_ctx);
    fprintf(stderr, "cache miss!\n");
#endif

#if PROJ_VERSION_MAJOR == 6 && PROJ_VERSION_MINOR < 2
    if( strstr(in_str, "+proj=") && strstr(in_str, "+over") &&
        strstr(out_str, "+proj=") && strstr(out_str, "+over") &&
        strlen(in_str) < 400 && strlen(out_str) < 400 )
    {
        // Fixed per PROJ commit
        // https://github.com/OSGeo/PROJ/commit/78302efb70eb4b49610cda6a60bf9ce39b82264f
        // and
        // https://github.com/OSGeo/PROJ/commit/ae70b26b9cbae85a38d5b26533ba06da0ea13940
        // Fix for wcs_get_capabilities_tileindexmixedsrs_26711.xml and wcs_20_getcov_bands_name_new_reproject.dat
        char szPipeline[1024];
        strcpy(szPipeline, "+proj=pipeline");
        if( msProjIsGeographicCRS(in) )
        {
            strcat(szPipeline, " +step +proj=unitconvert +xy_in=deg +xy_out=rad");
        }
        strcat(szPipeline, " +step +inv ");
        strcat(szPipeline, in_str);
        strcat(szPipeline, " +step ");
        strcat(szPipeline, out_str);
        if( msProjIsGeographicCRS(out) )
        {
            strcat(szPipeline, " +step +proj=unitconvert +xy_in=rad +xy_out=deg");
        }
        /* We do not want datum=NAD83 to imply a transformation with towgs84=0,0,0 */
        {
            char* ptr = szPipeline;
            while(1)
            {
                ptr = strstr(ptr, " +datum=NAD83");
                if( !ptr )
                    break;
                memcpy(ptr, " +ellps=GRS80", 13);
            }
        }

        /* Remove +nadgrids=@null as it doesn't work if going outside of [-180,180] */
        /* Fixed per https://github.com/OSGeo/PROJ/commit/10a30bb539be1afb25952b19af8bbe72e1b13b56 */
        {
            char* ptr = strstr(szPipeline, " +nadgrids=@null");
            if( ptr )
                memcpy(ptr, "                ", 16);
        }

        pj_normalized = proj_create(in->proj_ctx->proj_ctx, szPipeline);
    }
    else
#endif
    {
#if PROJ_VERSION_MAJOR > 6 || (PROJ_VERSION_MAJOR == 6 && PROJ_VERSION_MINOR >= 2)
        pj_raw = proj_create_crs_to_crs_from_pj(in->proj_ctx->proj_ctx, in->proj, out->proj, NULL, NULL);
#else
        pj_raw = proj_create_crs_to_crs(in->proj_ctx->proj_ctx, in_str, out_str, NULL);
#endif
        if( !pj_raw )
        {
            msFree(in_str_for_cache);
            msFree(out_str_for_cache);
            return NULL;
        }
        pj_normalized = proj_normalize_for_visualization(in->proj_ctx->proj_ctx, pj_raw);
        proj_destroy(pj_raw);
    }
    if( !pj_normalized )
    {
        msFree(in_str_for_cache);
        msFree(out_str_for_cache);
        return NULL;
    }

    if( in->proj_ctx->proj_ctx == out->proj_ctx->proj_ctx )
    {
        /* Insert entry into cache */
        int i;
        pjCacheEntry* pj_cache = in->proj_ctx->pj_cache;
        if( in->proj_ctx->pj_cache_size < PJ_CACHE_ENTRY_SIZE )
        {
            i = in->proj_ctx->pj_cache_size;
            assert ( pj_cache[i].inStr == NULL );
            in->proj_ctx->pj_cache_size ++;
        }
        else
        {
            i = 0;
            /* Evict oldest entry */
            msFree(pj_cache[PJ_CACHE_ENTRY_SIZE - 1].inStr);
            msFree(pj_cache[PJ_CACHE_ENTRY_SIZE - 1].outStr);
            proj_destroy(pj_cache[PJ_CACHE_ENTRY_SIZE - 1].pj);
            memmove(&pj_cache[1], &pj_cache[0],
                    (PJ_CACHE_ENTRY_SIZE - 1) * sizeof(pjCacheEntry));
        }
        pj_cache[i].inStr = msStrdup(in_str_for_cache);
        pj_cache[i].outStr = msStrdup(out_str_for_cache);
        pj_cache[i].pj = pj_normalized;
        *pbFreePJ = FALSE;
    }
    else
    {
        *pbFreePJ = TRUE;
    }

    msFree(in_str_for_cache);
    msFree(out_str_for_cache);

    return pj_normalized;
}

/************************************************************************/
/*                        getBaseGeographicCRS()                        */
/************************************************************************/

/* Return to be freed with proj_destroy() */
static PJ* getBaseGeographicCRS(projectionObj* in)
{
    PJ_CONTEXT* ctxt;
    PJ_TYPE type;
    assert(in && in->proj);
    ctxt = in->proj_ctx->proj_ctx;
    type = proj_get_type(in->proj);
    if( type == PJ_TYPE_PROJECTED_CRS )
    {
        return proj_get_source_crs(ctxt, in->proj);
    }
    if( type == PJ_TYPE_BOUND_CRS )
    {
        /* If it is a boundCRS of a projectedCRS, extract the geographicCRS
         * from the projectedCRS, and rewrap it in a boundCRS */
        PJ* source_crs = proj_get_source_crs(ctxt, in->proj);
        PJ* geog_source_crs;
        PJ* hub_crs;
        PJ* transf;
        PJ* ret;
        if( proj_get_type(source_crs) != PJ_TYPE_PROJECTED_CRS )
        {
            proj_destroy(source_crs);
            return NULL;
        }
        geog_source_crs = proj_get_source_crs(ctxt, source_crs);
        proj_destroy(source_crs);
        hub_crs = proj_get_target_crs(ctxt, in->proj);
        transf = proj_crs_get_coordoperation(ctxt, in->proj);
        ret = proj_crs_create_bound_crs(ctxt, geog_source_crs,
                                        hub_crs, transf);
        proj_destroy(geog_source_crs);
        proj_destroy(hub_crs);
        proj_destroy(transf);
        return ret;

    }
    return NULL;
}

/************************************************************************/
/*                          msProjErrorLogger()                         */
/************************************************************************/

static void msProjErrorLogger(void * user_data,
                             int level, const char * message)
{
    (void)user_data;
#if PROJ_VERSION_MAJOR >= 6    
    if( level == PJ_LOG_ERROR && msGetGlobalDebugLevel() >= MS_DEBUGLEVEL_VV )
#else
    if( level == PJ_LOG_ERROR )
#endif
    {
        msDebug( "PROJ: Error: %s\n", message );
    }
    else if( level == PJ_LOG_DEBUG )
    {
        msDebug( "PROJ: Debug: %s\n", message );
    }
}

/************************************************************************/
/*                        msProjectionContextCreate()                   */
/************************************************************************/

projectionContext* msProjectionContextCreate(void)
{
    projectionContext* ctx = (projectionContext*)msSmallCalloc(1, sizeof(projectionContext));
    ctx->proj_ctx = proj_context_create();
    if( ctx->proj_ctx == NULL )
    {
        msFree(ctx);
        return NULL;
    }
    ctx->ref_count = 1;
    proj_context_use_proj4_init_rules(ctx->proj_ctx, TRUE);
    proj_log_func (ctx->proj_ctx, NULL, msProjErrorLogger);
    return ctx;
}

/************************************************************************/
/*                        msProjectionContextUnref()                    */
/************************************************************************/

void msProjectionContextUnref(projectionContext* ctx)
{
    if( !ctx )
        return;
    --ctx->ref_count;
    if( ctx->ref_count == 0 )
    {
        int i;
        for( i = 0; i < ctx->pj_cache_size; i++ )
        {
            msFree(ctx->pj_cache[i].inStr);
            msFree(ctx->pj_cache[i].outStr);
            proj_destroy(ctx->pj_cache[i].pj);
        }
        proj_context_destroy(ctx->proj_ctx);
        msFree(ctx);
    }
}

/************************************************************************/
/*                        msProjectCreateReprojector()                  */
/************************************************************************/

reprojectionObj* msProjectCreateReprojector(projectionObj* in, projectionObj* out)
{
    reprojectionObj* obj = (reprojectionObj*)msSmallCalloc(1, sizeof(reprojectionObj));
    obj->in = in;
    obj->out = out;
    obj->should_do_line_cutting = -1;

    /* -------------------------------------------------------------------- */
    /*      If the source and destination are simple and equal, then do     */
    /*      nothing.                                                        */
    /* -------------------------------------------------------------------- */
    if( in && in->numargs == 1 && out && out->numargs == 1
        && strcmp(in->args[0],out->args[0]) == 0 ) {
        /* do nothing, no transformation required */
    }
    /* -------------------------------------------------------------------- */
    /*      If we have a fully defined input coordinate system and          */
    /*      output coordinate system, then we will use createNormalizedPJ   */
    /* -------------------------------------------------------------------- */
    else if( in && in->proj && out && out->proj ) {
        PJ* pj = createNormalizedPJ(in, out, &(obj->bFreePJ));
        if( !pj )
        {
            msFree(obj);
            return NULL;
        }
        obj->pj = pj;
    }

    /* nothing to do if the other coordinate system is also lat/long */
    else if( (in == NULL || in->proj == NULL) &&
             (out == NULL || out->proj == NULL || msProjIsGeographicCRS(out) ))
    {
        /* do nothing */
    }
    else if( out == NULL && in != NULL && msProjIsGeographicCRS(in) )
    {
        /* do nothing */
    }

    /* -------------------------------------------------------------------- */
    /*      Otherwise we assume that the NULL projectionObj is supposed to be */
    /*      lat/long in the same datum as the other projectionObj.  This    */
    /*      is essentially a backwards compatibility mode.                  */
    /* -------------------------------------------------------------------- */
    else {
        PJ* pj = NULL;

        if( (in==NULL || in->proj==NULL) && out && out->proj) { /* input coordinates are lat/lon */
            PJ* source_crs = getBaseGeographicCRS(out);
            projectionObj in_modified;
            memset(&in_modified, 0, sizeof(in_modified));

            in_modified.proj_ctx = out->proj_ctx;
            in_modified.proj = source_crs;
            pj = createNormalizedPJ(&in_modified, out, &(obj->bFreePJ));
            proj_destroy(source_crs);
        } else if( /* (out==NULL || out->proj==NULL) && */ in && in->proj )  {
            PJ* target_crs = getBaseGeographicCRS(in);
            projectionObj out_modified;
            memset(&out_modified, 0, sizeof(out_modified));

            out_modified.proj_ctx = in->proj_ctx;
            out_modified.proj = target_crs;
            pj = createNormalizedPJ(in, &out_modified, &(obj->bFreePJ));
            proj_destroy(target_crs);
        }
        if( !pj )
        {
            msFree(obj);
            return NULL;
        }
        obj->pj = pj;
    }

    return obj;
}

/************************************************************************/
/*                      msProjectDestroyReprojector()                   */
/************************************************************************/

void msProjectDestroyReprojector(reprojectionObj* reprojector)
{
    if( !reprojector )
        return;
    if( reprojector->bFreePJ )
        proj_destroy(reprojector->pj);
    msFreeShape(&(reprojector->splitShape));
    msFree(reprojector);
}

/************************************************************************/
/*                       msProjectTransformPoints()                     */
/************************************************************************/

int msProjectTransformPoints( reprojectionObj* reprojector,
                              int npoints, double* x, double* y )
{
    proj_trans_generic (reprojector->pj, PJ_FWD,
                        x, sizeof(double), npoints,
                        y, sizeof(double), npoints,
                        NULL, 0, 0,
                        NULL, 0, 0 );
    return MS_SUCCESS;
}

#else

/************************************************************************/
/*                        msProjectionContextCreate()                   */
/************************************************************************/

projectionContext* msProjectionContextCreate(void)
{
    return NULL;
}

/************************************************************************/
/*                        msProjectionContextUnref()                    */
/************************************************************************/

void msProjectionContextUnref(projectionContext* ctx)
{
    (void)ctx;
}

struct reprojectionObj
{
    projectionObj* in;
    projectionObj* out;
    int should_do_line_cutting;
    shapeObj splitShape;
    int no_op;
};

/************************************************************************/
/*                        msProjectCreateReprojector()                  */
/************************************************************************/

reprojectionObj* msProjectCreateReprojector(projectionObj* in, projectionObj* out)
{
    reprojectionObj* obj;
    obj = (reprojectionObj*)msSmallCalloc(1, sizeof(reprojectionObj));
    obj->in = in;
    obj->out = out;
    obj->should_do_line_cutting = -1;

    /* -------------------------------------------------------------------- */
    /*      If the source and destination are equal, then do nothing.       */
    /* -------------------------------------------------------------------- */
    if( in && out && in->numargs == out->numargs && in->numargs > 0
        && strcmp(in->args[0],out->args[0]) == 0 )
    {
        int i;
        obj->no_op = MS_TRUE;
        for( i = 1; i < in->numargs; i++ )
        {
            if( strcmp(in->args[i],out->args[i]) != 0 ) {
                obj->no_op = MS_FALSE;
                break;
            }
        }
    }

    /* -------------------------------------------------------------------- */
    /*      If we have a fully defined input coordinate system and          */
    /*      output coordinate system, then we will use pj_transform         */
    /* -------------------------------------------------------------------- */
    else if( in && in->proj && out && out->proj ) {
        /* do nothing for now */
    }
    /* nothing to do if the other coordinate system is also lat/long */
    else if( in == NULL && (out == NULL || msProjIsGeographicCRS(out) ))
    {
        obj->no_op = MS_TRUE;
    }
    else if( out == NULL && in != NULL && msProjIsGeographicCRS(in) )
    {
        obj->no_op = MS_TRUE;
    }
    else if( (in == NULL || in->proj == NULL) &&
             (out == NULL || out->proj == NULL) )
    {
        msProjectDestroyReprojector(obj);
        return NULL;
    }
    return obj;
}

/************************************************************************/
/*                      msProjectDestroyReprojector()                   */
/************************************************************************/

void msProjectDestroyReprojector(reprojectionObj* reprojector)
{
    if( !reprojector )
        return;
    msFreeShape(&(reprojector->splitShape));
    msFree(reprojector);
}

#endif

/*
** Initialize, load and free a projectionObj structure
*/
int msInitProjection(projectionObj *p)
{
  p->gt.need_geotransform = MS_FALSE;
  p->numargs = 0;
  p->args = NULL;
  p->wellknownprojection = wkp_none;
  p->proj = NULL;
  p->args = (char **)malloc(MS_MAXPROJARGS*sizeof(char *));
  MS_CHECK_ALLOC(p->args, MS_MAXPROJARGS*sizeof(char *), -1);
#if PROJ_VERSION_MAJOR >= 6
  p->proj_ctx = NULL;
#elif PJ_VERSION >= 480
  p->proj_ctx = NULL;
#endif
  return(0);
}

void msFreeProjection(projectionObj *p)
{
#if PROJ_VERSION_MAJOR >= 6
  proj_destroy(p->proj);
  p->proj = NULL;
  msProjectionContextUnref(p->proj_ctx);
  p->proj_ctx = NULL;
#else
  if(p->proj) {
    pj_free(p->proj);
    p->proj = NULL;
  }
#if PJ_VERSION >= 480
  if(p->proj_ctx) {
    pj_ctx_free(p->proj_ctx);
    p->proj_ctx = NULL;
  }
#endif
#endif

  p->gt.need_geotransform = MS_FALSE;
  p->wellknownprojection = wkp_none;
  msFreeCharArray(p->args, p->numargs);
  p->args = NULL;
  p->numargs = 0;
}

void msFreeProjectionExceptContext(projectionObj *p)
{
#if PROJ_VERSION_MAJOR >= 6
  projectionContext* ctx = p->proj_ctx;
  p->proj_ctx = NULL;
  msFreeProjection(p);
  p->proj_ctx = ctx;
#else
  msFreeProjection(p);
#endif
}

/************************************************************************/
/*                 msProjectionInheritContextFrom()                     */
/************************************************************************/

void msProjectionInheritContextFrom(projectionObj *pDst, const projectionObj* pSrc)
{
#if PROJ_VERSION_MAJOR >= 6
    if( pDst->proj_ctx == NULL && pSrc->proj_ctx != NULL)
    {
        pDst->proj_ctx = pSrc->proj_ctx;
        pDst->proj_ctx->ref_count ++;
    }
#else
    (void)pDst;
    (void)pSrc;
#endif
}

/************************************************************************/
/*                      msProjectionSetContext()                        */
/************************************************************************/

void msProjectionSetContext(projectionObj *p, projectionContext* ctx)
{
#if PROJ_VERSION_MAJOR >= 6
    if( p->proj_ctx == NULL && ctx != NULL)
    {
        p->proj_ctx = ctx;
        p->proj_ctx->ref_count ++;
    }
#else
    (void)p;
    (void)ctx;
#endif
}

/*
** Handle OGC WMS/WFS AUTO projection in the format:
**    "AUTO:proj_id,units_id,lon0,lat0"
*/

static int _msProcessAutoProjection(projectionObj *p)
{
  char **args;
  int numargs, nProjId, nUnitsId, nZone;
  double dLat0, dLon0;
  const char *pszUnits = "m";
  char szProjBuf[512]="";

  /* WMS/WFS AUTO projection: "AUTO:proj_id,units_id,lon0,lat0" */
  args = msStringSplit(p->args[0], ',', &numargs);
  if (numargs != 4 ||
      (strncasecmp(args[0], "AUTO:", 5) != 0 &&
       strncasecmp(args[0], "AUTO2:", 6) != 0)) {
    msSetError(MS_PROJERR,
               "WMS/WFS AUTO/AUTO2 PROJECTION must be in the format "
               "'AUTO:proj_id,units_id,lon0,lat0' or 'AUTO2:crs_id,factor,lon0,lat0'(got '%s').\n",
               "_msProcessAutoProjection()", p->args[0]);
    return -1;
  }

  if (strncasecmp(args[0], "AUTO:", 5)==0)
    nProjId = atoi(args[0]+5);
  else
    nProjId = atoi(args[0]+6);

  nUnitsId = atoi(args[1]);
  dLon0 = atof(args[2]);
  dLat0 = atof(args[3]);


  /*There is no unit parameter for AUTO2. The 2nd parameter is
   factor. Set the units to always be meter*/
  if (strncasecmp(args[0], "AUTO2:", 6) == 0)
    nUnitsId = 9001;

  msFreeCharArray(args, numargs);

  /* Handle EPSG Units.  Only meters for now. */
  switch(nUnitsId) {
    case 9001:  /* Meters */
      pszUnits = "m";
      break;
    default:
      msSetError(MS_PROJERR,
                 "WMS/WFS AUTO PROJECTION: EPSG Units %d not supported.\n",
                 "_msProcessAutoProjection()", nUnitsId);
      return -1;
  }

  /* Build PROJ4 definition.
   * This is based on the definitions found in annex E of the WMS 1.1.1
   * spec and online at http://www.digitalearth.org/wmt/auto.html
   * The conversion from the original WKT definitions to PROJ4 format was
   * done using the MapScript setWKTProjection() function (based on OGR).
   */
  switch(nProjId) {
    case 42001: /** WGS 84 / Auto UTM **/
      nZone = (int) floor( (dLon0 + 180.0) / 6.0 ) + 1;
      sprintf( szProjBuf,
               "+proj=tmerc+lat_0=0+lon_0=%.16g+k=0.999600+x_0=500000"
               "+y_0=%.16g+ellps=WGS84+datum=WGS84+units=%s+type=crs",
               -183.0 + nZone * 6.0,
               (dLat0 >= 0.0) ? 0.0 : 10000000.0,
               pszUnits);
      break;
    case 42002: /** WGS 84 / Auto Tr. Mercator **/
      sprintf( szProjBuf,
               "+proj=tmerc+lat_0=0+lon_0=%.16g+k=0.999600+x_0=500000"
               "+y_0=%.16g+ellps=WGS84+datum=WGS84+units=%s+type=crs",
               dLon0,
               (dLat0 >= 0.0) ? 0.0 : 10000000.0,
               pszUnits);
      break;
    case 42003: /** WGS 84 / Auto Orthographic **/
      sprintf( szProjBuf,
               "+proj=ortho+lon_0=%.16g+lat_0=%.16g+x_0=0+y_0=0"
               "+ellps=WGS84+datum=WGS84+units=%s+type=crs",
               dLon0, dLat0, pszUnits );
      break;
    case 42004: /** WGS 84 / Auto Equirectangular **/
      /* Note that we have to pass lon_0 as lon_ts for this one to */
      /* work.  Either a PROJ4 bug or a PROJ4 documentation issue. */
      sprintf( szProjBuf,
               "+proj=eqc+lon_ts=%.16g+lat_ts=%.16g+x_0=0+y_0=0"
               "+ellps=WGS84+datum=WGS84+units=%s+type=crs",
               dLon0, dLat0, pszUnits);
      break;
    case 42005: /** WGS 84 / Auto Mollweide **/
      sprintf( szProjBuf,
               "+proj=moll+lon_0=%.16g+x_0=0+y_0=0+ellps=WGS84"
               "+datum=WGS84+units=%s+type=crs",
               dLon0, pszUnits);
      break;
    default:
      msSetError(MS_PROJERR,
                 "WMS/WFS AUTO PROJECTION %d not supported.\n",
                 "_msProcessAutoProjection()", nProjId);
      return -1;
  }

  /* msDebug("%s = %s\n", p->args[0], szProjBuf); */

  /* OK, pass the definition to pj_init() */
  args = msStringSplit(szProjBuf, '+', &numargs);

#if PROJ_VERSION_MAJOR >= 6
  if( !(p->proj = proj_create_argv(p->proj_ctx->proj_ctx, numargs, args)) ) {
    int l_pj_errno = proj_context_errno (p->proj_ctx->proj_ctx);
    msSetError(MS_PROJERR, "proj error \"%s\" for \"%s\"",
               "msProcessProjection()", proj_errno_string(l_pj_errno), szProjBuf) ;
    return(-1);
  }
#else
  msAcquireLock( TLOCK_PROJ );
  if( !(p->proj = pj_init(numargs, args)) ) {
    int *pj_errno_ref = pj_get_errno_ref();
    msReleaseLock( TLOCK_PROJ );
    msSetError(MS_PROJERR, "proj error \"%s\" for \"%s\"",
               "msProcessProjection()", pj_strerrno(*pj_errno_ref), szProjBuf) ;
    return(-1);
  }
  msReleaseLock( TLOCK_PROJ );
#endif

  msFreeCharArray(args, numargs);

  return(0);
}

int msProcessProjection(projectionObj *p)
{
  assert( p->proj == NULL );

  if( strcasecmp(p->args[0],"GEOGRAPHIC") == 0 ) {
    msSetError(MS_PROJERR,
               "PROJECTION 'GEOGRAPHIC' no longer supported.\n"
               "Provide explicit definition.\n"
               "ie. proj=latlong\n"
               "    ellps=clrk66\n",
               "msProcessProjection()");
    return(-1);
  }

  if (strcasecmp(p->args[0], "AUTO") == 0) {
    p->proj = NULL;
    return 0;
  }

#if PROJ_VERSION_MAJOR >= 6
  if( p->proj_ctx == NULL )
  {
    p->proj_ctx = msProjectionContextCreate();
    if( p->proj_ctx == NULL )
    {
        return -1;
    }
  }
  if( p->proj_ctx->ms_proj_lib_change_counter != ms_proj_lib_change_counter )
  {
    msAcquireLock( TLOCK_PROJ );
    p->proj_ctx->ms_proj_lib_change_counter = ms_proj_lib_change_counter;
    {
        const char* const paths[1] = { ms_proj_lib };
        proj_context_set_search_paths(p->proj_ctx->proj_ctx, 1, ms_proj_lib ? paths : NULL);
    }
    msReleaseLock( TLOCK_PROJ );
  }
#endif

  if (strncasecmp(p->args[0], "AUTO:", 5) == 0 ||
      strncasecmp(p->args[0], "AUTO2:", 6) == 0) {
    /* WMS/WFS AUTO projection: "AUTO:proj_id,units_id,lon0,lat0" */
    /*WMS 1.3.0: AUTO2:auto_crs_id,factor,lon0,lat0*/
    return _msProcessAutoProjection(p);
  }

#if PROJ_VERSION_MAJOR >= 6
  if( p->numargs == 1 && strncmp(p->args[0], "init=", 5) != 0 )
  {
      /* Deal e.g. with EPSG:XXXX or ESRI:XXXX */
      if( !(p->proj = proj_create_argv(p->proj_ctx->proj_ctx, p->numargs, p->args)) ) {
          int l_pj_errno = proj_context_errno (p->proj_ctx->proj_ctx);
          msSetError(MS_PROJERR, "proj error \"%s\" for \"%s\"",
                     "msProcessProjection()", proj_errno_string(l_pj_errno), p->args[0]) ;
          return(-1);
      }
  }
  else
  {
      char** args = (char**)msSmallMalloc(sizeof(char*) * (p->numargs+1));
      memcpy(args, p->args, sizeof(char*) * p->numargs);

#if PROJ_VERSION_MAJOR == 6 && PROJ_VERSION_MINOR < 2
      /* PROJ lookups are faster with EPSG in uppercase. Fixed in PROJ 6.2 */
      /* Do that only for those versions, as it can create confusion if using */
      /* a real old-style 'epsg' file... */
      char szTemp[24];
      if( p->numargs && strncmp(args[0], "init=epsg:", strlen("init=epsg:")) == 0 &&
          strlen(args[0]) < 24)
      {
          strcpy(szTemp, "init=EPSG:");
          strcat(szTemp, args[0] + strlen("init=epsg:"));
          args[0] = szTemp;
      }
#endif

      args[p->numargs] = (char*) "type=crs";
#if 0
      for( int i = 0; i <  p->numargs + 1; i++ )
          fprintf(stderr, "%s ", args[i]);
      fprintf(stderr, "\n");
#endif
      if( !(p->proj = proj_create_argv(p->proj_ctx->proj_ctx, p->numargs + 1, args)) ) {
          int l_pj_errno = proj_context_errno (p->proj_ctx->proj_ctx);
          if(p->numargs>1) {
            msSetError(MS_PROJERR, "proj error \"%s\" for \"%s:%s\"",
                        "msProcessProjection()", proj_errno_string(l_pj_errno), p->args[0],p->args[1]) ;
          } else {
            msSetError(MS_PROJERR, "proj error \"%s\" for \"%s\"",
                        "msProcessProjection()", proj_errno_string(l_pj_errno), p->args[0]) ;
          }
          free(args);
          return(-1);
      }
      free(args);
  }
#else
  msAcquireLock( TLOCK_PROJ );
#if PJ_VERSION < 480
  if( !(p->proj = pj_init(p->numargs, p->args)) ) {
#else
  p->proj_ctx = pj_ctx_alloc();
  if( !(p->proj=pj_init_ctx(p->proj_ctx, p->numargs, p->args)) ) {
#endif

    int *pj_errno_ref = pj_get_errno_ref();
    msReleaseLock( TLOCK_PROJ );
    if(p->numargs>1) {
      msSetError(MS_PROJERR, "proj error \"%s\" for \"%s:%s\"",
                 "msProcessProjection()", pj_strerrno(*pj_errno_ref), p->args[0],p->args[1]) ;
    } else {
      msSetError(MS_PROJERR, "proj error \"%s\" for \"%s\"",
                 "msProcessProjection()", pj_strerrno(*pj_errno_ref), p->args[0]) ;
    }
    return(-1);
  }

  msReleaseLock( TLOCK_PROJ );
#endif

#ifdef USE_PROJ_FASTPATHS
  if(strcasestr(p->args[0],"epsg:4326")) {
    p->wellknownprojection = wkp_lonlat;
  } else if(strcasestr(p->args[0],"epsg:3857")) {
    p->wellknownprojection = wkp_gmerc;
  } else {
    p->wellknownprojection = wkp_none;
  }
#endif


  return(0);
}

/************************************************************************/
/*                           int msIsAxisInverted                       */
/*      Check to see if we should invert the axis.                       */
/*                                                                      */
/************************************************************************/
int msIsAxisInverted(int epsg_code)
{
  const unsigned int row = epsg_code / 8;
  const unsigned char index = epsg_code % 8;

  /*check the static table*/
  if ((row < sizeof(axisOrientationEpsgCodes)) && (axisOrientationEpsgCodes[row] & (1 << index)))
    return MS_TRUE;
  else
    return MS_FALSE;
}

/************************************************************************/
/*                           msProjectPoint()                           */
/************************************************************************/
int msProjectPoint(projectionObj *in, projectionObj *out, pointObj *point)
{
    int ret;
    reprojectionObj* reprojector = msProjectCreateReprojector(in, out);
    if( !reprojector )
        return MS_FAILURE;
    ret = msProjectPointEx(reprojector, point);
    msProjectDestroyReprojector(reprojector);
    return ret;
}

/************************************************************************/
/*                           msProjectPointEx()                         */
/************************************************************************/
int msProjectPointEx(reprojectionObj* reprojector, pointObj *point)
{
  projectionObj* in = reprojector->in;
  projectionObj* out = reprojector->out;

  if( in && in->gt.need_geotransform ) {
    double x_out, y_out;

    x_out = in->gt.geotransform[0]
            + in->gt.geotransform[1] * point->x
            + in->gt.geotransform[2] * point->y;
    y_out = in->gt.geotransform[3]
            + in->gt.geotransform[4] * point->x
            + in->gt.geotransform[5] * point->y;

    point->x = x_out;
    point->y = y_out;
  }

#if PROJ_VERSION_MAJOR >= 6
  if( reprojector->pj ) {
    PJ_COORD c;
    c.xyzt.x = point->x;
    c.xyzt.y = point->y;
    c.xyzt.z = 0;
    c.xyzt.t = 0;
    c = proj_trans (reprojector->pj, PJ_FWD, c);
    if( c.xyzt.x == HUGE_VAL || c.xyzt.y == HUGE_VAL ) {
      return MS_FAILURE;
    }
    point->x = c.xyzt.x;
    point->y = c.xyzt.y;
  }
#else
  if( reprojector->no_op ) {
    /* do nothing, no transformation required */
  }

  /* -------------------------------------------------------------------- */
  /*      If we have a fully defined input coordinate system and          */
  /*      output coordinate system, then we will use pj_transform.        */
  /* -------------------------------------------------------------------- */
  else if( in && in->proj && out && out->proj ) {
    int error;
    double  z = 0.0;

    if( pj_is_latlong(in->proj) ) {
      point->x *= DEG_TO_RAD;
      point->y *= DEG_TO_RAD;
    }

#if PJ_VERSION < 480
    msAcquireLock( TLOCK_PROJ );
#endif
    error = pj_transform( in->proj, out->proj, 1, 0,
                          &(point->x), &(point->y), &z );
#if PJ_VERSION < 480
    msReleaseLock( TLOCK_PROJ );
#endif

    if( error || point->x == HUGE_VAL || point->y == HUGE_VAL ) {
//      msSetError(MS_PROJERR,"proj says: %s","msProjectPoint()",pj_strerrno(error));
      return MS_FAILURE;
    }

    if( pj_is_latlong(out->proj) ) {
      point->x *= RAD_TO_DEG;
      point->y *= RAD_TO_DEG;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Otherwise we fallback to using pj_fwd() or pj_inv() and         */
  /*      assuming that the NULL projectionObj is supposed to be          */
  /*      lat/long in the same datum as the other projectionObj.  This    */
  /*      is essentially a backwards compatibility mode.                  */
  /* -------------------------------------------------------------------- */
  else {
    projUV p;

    p.u = point->x;
    p.v = point->y;

    if(in==NULL || in->proj==NULL) { /* input coordinates are lat/lon */
      p.u *= DEG_TO_RAD; /* convert to radians */
      p.v *= DEG_TO_RAD;
      p = pj_fwd(p, out->proj);
    } else /* if(out==NULL || out->proj==NULL) */ { /* output coordinates are lat/lon */
      p = pj_inv(p, in->proj);
      p.u *= RAD_TO_DEG; /* convert to decimal degrees */
      p.v *= RAD_TO_DEG;
    }

    point->x = p.u;
    point->y = p.v;
    if( point->x == HUGE_VAL || point->y == HUGE_VAL ) {
      return MS_FAILURE;
    }
  }
#endif

  if( out && out->gt.need_geotransform ) {
    double x_out, y_out;

    x_out = out->gt.invgeotransform[0]
            + out->gt.invgeotransform[1] * point->x
            + out->gt.invgeotransform[2] * point->y;
    y_out = out->gt.invgeotransform[3]
            + out->gt.invgeotransform[4] * point->x
            + out->gt.invgeotransform[5] * point->y;

    point->x = x_out;
    point->y = y_out;
  }

  return(MS_SUCCESS);
}

/************************************************************************/
/*                         msProjectGrowRect()                          */
/************************************************************************/
static void msProjectGrowRect(reprojectionObj* reprojector,
                              rectObj *prj_rect,
                              pointObj *prj_point, int *failure )

{
  if( msProjectPointEx(reprojector, prj_point) == MS_SUCCESS ) {
      prj_rect->miny = MS_MIN(prj_rect->miny, prj_point->y);
      prj_rect->maxy = MS_MAX(prj_rect->maxy, prj_point->y);
      prj_rect->minx = MS_MIN(prj_rect->minx, prj_point->x);
      prj_rect->maxx = MS_MAX(prj_rect->maxx, prj_point->x);
  } else
    (*failure)++;
}

/************************************************************************/
/*                          msProjectSegment()                          */
/*                                                                      */
/*      Interpolate along a line segment for which one end              */
/*      reprojects and the other end does not.  Finds the horizon.      */
/************************************************************************/
static int msProjectSegment( reprojectionObj* reprojector,
                             pointObj *start, pointObj *end )

{
  pointObj testPoint, subStart, subEnd;

  /* -------------------------------------------------------------------- */
  /*      Without loss of generality we assume the first point            */
  /*      reprojects, and the second doesn't.  If that is not the case    */
  /*      then re-call with the points reversed.                          */
  /* -------------------------------------------------------------------- */
  testPoint = *start;
  if( msProjectPointEx( reprojector, &testPoint ) == MS_FAILURE ) {
    testPoint = *end;
    if( msProjectPointEx( reprojector, &testPoint ) == MS_FAILURE )
      return MS_FAILURE;
    else
      return msProjectSegment( reprojector, end, start );
  }

  /* -------------------------------------------------------------------- */
  /*      We will apply a binary search till we are within out            */
  /*      tolerance.                                                      */
  /* -------------------------------------------------------------------- */
  subStart = *start;
  subEnd = *end;

#define TOLERANCE 0.01

  while( fabs(subStart.x - subEnd.x)
         + fabs(subStart.y - subEnd.y) > TOLERANCE ) {
    pointObj midPoint = {0};

    midPoint.x = (subStart.x + subEnd.x) * 0.5;
    midPoint.y = (subStart.y + subEnd.y) * 0.5;

    testPoint = midPoint;

    if( msProjectPointEx( reprojector, &testPoint ) == MS_FAILURE ) {
      if (midPoint.x == subEnd.x && midPoint.y == subEnd.y)
        return MS_FAILURE; /* break infinite loop */

      subEnd = midPoint;
    }
    else {
      if (midPoint.x == subStart.x && midPoint.y == subStart.y)
        return MS_FAILURE; /* break infinite loop */

      subStart = midPoint;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Now reproject the end points and return.                        */
  /* -------------------------------------------------------------------- */
  *end = subStart;

  if( msProjectPointEx( reprojector, end ) == MS_FAILURE
      || msProjectPointEx( reprojector, start ) == MS_FAILURE )
    return MS_FAILURE;
  else
    return MS_SUCCESS;
}

/************************************************************************/
/*                   msProjectShapeShouldDoLineCutting()                */
/************************************************************************/

/* Detect projecting from north polar stereographic to longlat or EPSG:3857 */
#ifdef USE_GEOS
static int msProjectShapeShouldDoLineCutting(reprojectionObj* reprojector)
{
    if( reprojector->should_do_line_cutting >= 0)
        return reprojector->should_do_line_cutting;

    projectionObj *in = reprojector->in;
    projectionObj *out = reprojector->out;
    if( !(!in->gt.need_geotransform && !msProjIsGeographicCRS(in) &&
            (msProjIsGeographicCRS(out) ||
            (out->numargs == 1 && strcmp(out->args[0], "init=epsg:3857") == 0))) )
    {
        reprojector->should_do_line_cutting = MS_FALSE;
        return MS_FALSE;
    }

    int srcIsPolar;
    double extremeLongEasting;
    if( msProjIsGeographicCRS(out) )
    {
        pointObj p;
        double gt3 = out->gt.need_geotransform ? out->gt.geotransform[3] : 0.0;
        double gt4 = out->gt.need_geotransform ? out->gt.geotransform[4] : 0.0;
        double gt5 = out->gt.need_geotransform ? out->gt.geotransform[5] : 1.0;
        p.x = 0.0;
        p.y = 0.0;
        srcIsPolar =  msProjectPointEx(reprojector, &p) == MS_SUCCESS &&
                        fabs(gt3 + p.x * gt4 + p.y * gt5 - 90) < 1e-8;
        extremeLongEasting = 180;
    }
    else
    {
        pointObj p1;
        pointObj p2;
        double gt1 = out->gt.need_geotransform ? out->gt.geotransform[1] : 1.0;
        p1.x = 0.0;
        p1.y = -0.1;
        p2.x = 0.0;
        p2.y = 0.1;
        srcIsPolar =  msProjectPointEx(reprojector, &p1) == MS_SUCCESS &&
                      msProjectPointEx(reprojector, &p2) == MS_SUCCESS &&
                     fabs((p1.x - p2.x) * gt1) > 20e6;
        extremeLongEasting = 20037508.3427892;
    }
    if( !srcIsPolar )
    {
        reprojector->should_do_line_cutting = MS_FALSE;
        return MS_FALSE;
    }

    pointObj p = {0}; // initialize
    double invgt0 = out->gt.need_geotransform ? out->gt.invgeotransform[0] : 0.0;
    double invgt1 = out->gt.need_geotransform ? out->gt.invgeotransform[1] : 1.0;
    double invgt3 = out->gt.need_geotransform ? out->gt.invgeotransform[3] : 0.0;
    double invgt4 = out->gt.need_geotransform ? out->gt.invgeotransform[4] : 0.0;

    lineObj newLine = {0,NULL};
    const double EPS = 1e-10;

    p.x = invgt0 + -extremeLongEasting * (1 - EPS) * invgt1;
    p.y = invgt3 + -extremeLongEasting * (1 - EPS) * invgt4;
    /* coverity[swapped_arguments] */
    msProjectPoint(out, in, &p);
    pointObj first = p;
    msAddPointToLine(&newLine, &p );

    p.x = invgt0 + extremeLongEasting * (1 - EPS) * invgt1;
    p.y = invgt3 + extremeLongEasting * (1 - EPS) * invgt4;
    /* coverity[swapped_arguments] */
    msProjectPoint(out, in, &p);
    msAddPointToLine(&newLine, &p );

    p.x = 0;
    p.y = 0;
    msAddPointToLine(&newLine, &p );

    msAddPointToLine(&newLine, &first );

    msInitShape(&(reprojector->splitShape));
    reprojector->splitShape.type = MS_SHAPE_POLYGON;
    msAddLineDirectly(&(reprojector->splitShape), &newLine);

    reprojector->should_do_line_cutting = MS_TRUE;
    return MS_TRUE;
}
#endif

/************************************************************************/
/*                         msProjectShapeLine()                         */
/*                                                                      */
/*      Reproject a single linestring, potentially splitting into       */
/*      more than one line string if there are over the horizon         */
/*      portions.                                                       */
/*                                                                      */
/*      For polygons, no splitting takes place, but over the horizon    */
/*      points are clipped, and one segment is run from the fall        */
/*      over the horizon point to the come back over the horizon point. */
/************************************************************************/

static int
msProjectShapeLine(reprojectionObj* reprojector,
                   shapeObj *shape, int line_index)

{
  int i;
  pointObj  lastPoint, thisPoint, wrkPoint;
  lineObj *line = shape->line + line_index;
  lineObj *line_out = line;
  int valid_flag = 0; /* 1=true, -1=false, 0=unknown */
  int numpoints_in = line->numpoints;
  int line_alloc = numpoints_in;
  int wrap_test;
  projectionObj *in = reprojector->in;
  projectionObj *out = reprojector->out;

#ifdef USE_PROJ_FASTPATHS
#define MAXEXTENT 20037508.34
#define M_PIby360 .0087266462599716479
#define MAXEXTENTby180 111319.4907777777777777777
#define p_x line->point[i].x
#define p_y line->point[i].y
  if(in->wellknownprojection == wkp_lonlat && out->wellknownprojection == wkp_gmerc) {
    for( i = line->numpoints-1; i >= 0; i-- ) {
      p_x *= MAXEXTENTby180;
      p_y = log(tan((90 + p_y) * M_PIby360)) * MS_RAD_TO_DEG;
      p_y *= MAXEXTENTby180;
      if (p_x > MAXEXTENT) p_x = MAXEXTENT;
      if (p_x < -MAXEXTENT) p_x = -MAXEXTENT;
      if (p_y > MAXEXTENT) p_y = MAXEXTENT;
      if (p_y < -MAXEXTENT) p_y = -MAXEXTENT;
    }
    return MS_SUCCESS;
  }
  if(in->wellknownprojection == wkp_gmerc && out->wellknownprojection == wkp_lonlat) {
    for( i = line->numpoints-1; i >= 0; i-- ) {
      if (p_x > MAXEXTENT) p_x = MAXEXTENT;
      else if (p_x < -MAXEXTENT) p_x = -MAXEXTENT;
      if (p_y > MAXEXTENT) p_y = MAXEXTENT;
      else if (p_y < -MAXEXTENT) p_y = -MAXEXTENT;
      p_x = (p_x / MAXEXTENT) * 180;
      p_y = (p_y / MAXEXTENT) * 180;
      p_y = MS_RAD_TO_DEG * (2 * atan(exp(p_y * MS_DEG_TO_RAD)) - MS_PI2);
    }
    msComputeBounds( shape ); /* fixes bug 1586 */
    return MS_SUCCESS;
  }
#undef p_x
#undef p_y
#endif

#ifdef USE_GEOS
  if( shape->type == MS_SHAPE_LINE &&
      msProjectShapeShouldDoLineCutting(reprojector) )
  {
    shapeObj tmpShapeInputLine;
    msInitShape(&tmpShapeInputLine);
    tmpShapeInputLine.type = MS_SHAPE_LINE;
    tmpShapeInputLine.numlines = 1;
    tmpShapeInputLine.line = line;

    shapeObj* diff = NULL;
    if( msGEOSIntersects(&tmpShapeInputLine, &(reprojector->splitShape)) )
    {
        diff = msGEOSDifference(&tmpShapeInputLine, &(reprojector->splitShape));
    }

    tmpShapeInputLine.numlines = 0;
    tmpShapeInputLine.line = NULL;
    msFreeShape(&tmpShapeInputLine);

    if( diff )
    {
        for(int j = 0; j < diff->numlines; j++ )
        {
            for( i=0; i < diff->line[j].numpoints; i++ ) {
                msProjectPointEx(reprojector, &(diff->line[j].point[i]));
            }
            if( j == 0 )
            {
                line_out->numpoints = diff->line[j].numpoints;
                memcpy(line_out->point, diff->line[0].point, sizeof(pointObj) * line_out->numpoints);
            }
            else
            {
                msAddLineDirectly(shape, &(diff->line[j]));
            }
        }
        msFreeShape(diff);
        msFree(diff);
        return MS_SUCCESS;
    }
  }
#endif

  wrap_test = out != NULL && out->proj != NULL && msProjIsGeographicCRS(out)
              && !msProjIsGeographicCRS(in);

  line->numpoints = 0;

  memset( &lastPoint, 0, sizeof(lastPoint) );

  /* -------------------------------------------------------------------- */
  /*      Loop over all input points in linestring.                       */
  /* -------------------------------------------------------------------- */
  for( i=0; i < numpoints_in; i++ ) {
    int ms_err;
    wrkPoint = thisPoint = line->point[i];

    ms_err = msProjectPointEx(reprojector, &wrkPoint );

    /* -------------------------------------------------------------------- */
    /*      Apply wrap logic.                                               */
    /* -------------------------------------------------------------------- */
    if( wrap_test && i > 0 && ms_err != MS_FAILURE ) {
      double dist;
      pointObj pt1Geo;

      if( line_out->numpoints > 0 )
        pt1Geo = line_out->point[line_out->numpoints-1];
      else
        pt1Geo = wrkPoint; /* this is a cop out */

      if( out->gt.need_geotransform && out->gt.geotransform[2] == 0 ) {
        dist = out->gt.geotransform[1] * (wrkPoint.x - pt1Geo.x);
      } else {
        dist = wrkPoint.x - pt1Geo.x;
      }

      if( fabs(dist) > 180.0
          && msTestNeedWrap( thisPoint, lastPoint,
                             pt1Geo, reprojector ) ) {
        if( out->gt.need_geotransform && out->gt.geotransform[2] == 0 ) {
          if( dist > 0.0 )
            wrkPoint.x -= 360.0 * out->gt.invgeotransform[1];
          else if( dist < 0.0 )
            wrkPoint.x += 360.0 * out->gt.invgeotransform[1];
        }
        else {
          if( dist > 0.0 )
            wrkPoint.x -= 360.0;
          else if( dist < 0.0 )
            wrkPoint.x += 360.0;
        }
      }
    }

    /* -------------------------------------------------------------------- */
    /*      Put result into output line with appropriate logic for          */
    /*      failure breaking lines, etc.                                    */
    /* -------------------------------------------------------------------- */
    if( ms_err == MS_FAILURE ) {
      /* We have started out invalid */
      if( i == 0 ) {
        valid_flag = -1;
      }

      /* valid data has ended, we need to work out the horizon */
      else if( valid_flag == 1 ) {
        pointObj startPoint, endPoint;

        startPoint = lastPoint;
        endPoint = thisPoint;
        if( msProjectSegment( reprojector, &startPoint, &endPoint )
            == MS_SUCCESS ) {
          line_out->point[line_out->numpoints++] = endPoint;
        }
        valid_flag = -1;
      }

      /* Still invalid ... */
      else if( valid_flag == -1 ) {
        /* do nothing */
      }
    }

    else {
      /* starting out valid. */
      if( i == 0 ) {
        line_out->point[line_out->numpoints++] = wrkPoint;
        valid_flag = 1;
      }

      /* Still valid, nothing special */
      else if( valid_flag == 1 ) {
        line_out->point[line_out->numpoints++] = wrkPoint;
      }

      /* we have come over the horizon, figure out where, start newline*/
      else {
        pointObj startPoint, endPoint;

        startPoint = lastPoint;
        endPoint = thisPoint;
        if( msProjectSegment( reprojector, &endPoint, &startPoint )
            == MS_SUCCESS ) {
          lineObj newLine;

          /* force pre-allocation of lots of points room */
          if( line_out->numpoints > 0
              && shape->type == MS_SHAPE_LINE ) {
            newLine.numpoints = numpoints_in - i + 1;
            newLine.point = line->point;
            msAddLine( shape, &newLine );

            /* new line is now lineout, but start without any points */
            line_out = shape->line + shape->numlines-1;

            line_out->numpoints = 0;

            /* the shape->line array is realloc, refetch "line" */
            line = shape->line + line_index;
          } else if( line_out == line
                     && line->numpoints >= i-2 ) {
            newLine.numpoints = numpoints_in;
            newLine.point = line->point;
            msAddLine( shape, &newLine );

            line = shape->line + line_index;

            line_out = shape->line + shape->numlines-1;
            line_out->numpoints = line->numpoints;
            line->numpoints = 0;

            /*
             * Now realloc this array large enough to hold all
             * the points we could possibly need to add.
             */
            line_alloc = line_alloc * 2;

            line_out->point = (pointObj *)
                              realloc(line_out->point,
                                      sizeof(pointObj) * line_alloc);
          }

          line_out->point[line_out->numpoints++] = startPoint;
        }
        line_out->point[line_out->numpoints++] = wrkPoint;
        valid_flag = 1;
      }
    }

    lastPoint = thisPoint;
  }

  /* -------------------------------------------------------------------- */
  /*      Make sure that polygons are closed, even if the trip over       */
  /*      the horizon left them unclosed.                                 */
  /* -------------------------------------------------------------------- */
  if( shape->type == MS_SHAPE_POLYGON
      && line_out->numpoints > 2
      && (line_out->point[0].x != line_out->point[line_out->numpoints-1].x
          || line_out->point[0].y != line_out->point[line_out->numpoints-1].y) ) {
    /* make a copy because msAddPointToLine can realloc the array */
    pointObj sFirstPoint = line_out->point[0];
    msAddPointToLine( line_out, &sFirstPoint );
  }

  return(MS_SUCCESS);
}

/************************************************************************/
/*                           msProjectShape()                           */
/************************************************************************/
int msProjectShape(projectionObj *in, projectionObj *out, shapeObj *shape)
{
    int ret;
    reprojectionObj* reprojector = msProjectCreateReprojector(in, out);
    if( !reprojector )
        return MS_FAILURE;
    ret = msProjectShapeEx(reprojector, shape);
    msProjectDestroyReprojector(reprojector);
    return ret;
}

/************************************************************************/
/*                          msProjectShapeEx()                          */
/************************************************************************/
int msProjectShapeEx(reprojectionObj* reprojector, shapeObj *shape)
{
  int i;
#ifdef USE_PROJ_FASTPATHS
  int j;

#define p_x shape->line[i].point[j].x
#define p_y shape->line[i].point[j].y
  if(in->wellknownprojection == wkp_lonlat && out->wellknownprojection == wkp_gmerc) {
    for( i = shape->numlines-1; i >= 0; i-- ) {
      for( j = shape->line[i].numpoints-1; j >= 0; j-- ) {
        p_x *= MAXEXTENTby180;
        p_y = log(tan((90 + p_y) * M_PIby360)) * MS_RAD_TO_DEG;
        p_y *= MAXEXTENTby180;
        if (p_x > MAXEXTENT) p_x = MAXEXTENT;
        else if (p_x < -MAXEXTENT) p_x = -MAXEXTENT;
        if (p_y > MAXEXTENT) p_y = MAXEXTENT;
        else if (p_y < -MAXEXTENT) p_y = -MAXEXTENT;
      }
    }
    msComputeBounds( shape ); /* fixes bug 1586 */
    return MS_SUCCESS;
  }
  if(in->wellknownprojection == wkp_gmerc && out->wellknownprojection == wkp_lonlat) {
    for( i = shape->numlines-1; i >= 0; i-- ) {
      for( j = shape->line[i].numpoints-1; j >= 0; j-- ) {
        if (p_x > MAXEXTENT) p_x = MAXEXTENT;
        else if (p_x < -MAXEXTENT) p_x = -MAXEXTENT;
        if (p_y > MAXEXTENT) p_y = MAXEXTENT;
        else if (p_y < -MAXEXTENT) p_y = -MAXEXTENT;
        p_x = (p_x / MAXEXTENT) * 180;
        p_y = (p_y / MAXEXTENT) * 180;
        p_y = MS_RAD_TO_DEG * (2 * atan(exp(p_y * MS_DEG_TO_RAD)) - MS_PI2);
      }
    }
    msComputeBounds( shape ); /* fixes bug 1586 */
    return MS_SUCCESS;
  }
#undef p_x
#undef p_y
#endif

  for( i = shape->numlines-1; i >= 0; i-- ) {
    if( shape->type == MS_SHAPE_LINE || shape->type == MS_SHAPE_POLYGON ) {
      if( msProjectShapeLine( reprojector, shape, i ) == MS_FAILURE )
        msShapeDeleteLine( shape, i );
    } else if( msProjectLineEx(reprojector, shape->line+i ) == MS_FAILURE ) {
      msShapeDeleteLine( shape, i );
    }
  }

  if( shape->numlines == 0 ) {
    msFreeShape( shape );
    return MS_FAILURE;
  } else {
    msComputeBounds( shape ); /* fixes bug 1586 */
    return(MS_SUCCESS);
  }
}

/************************************************************************/
/*                           msProjectLine()                            */
/************************************************************************/
int msProjectLine(projectionObj *in, projectionObj *out, lineObj *line)
{
    int ret;
    reprojectionObj* reprojector = msProjectCreateReprojector(in, out);
    if( !reprojector )
        return MS_FAILURE;
    ret = msProjectLineEx(reprojector, line);
    msProjectDestroyReprojector(reprojector);
    return ret;
}

/************************************************************************/
/*                         msProjectLineEx()                            */
/*                                                                      */
/*      This function is now normally only used for point data.         */
/*      msProjectShapeLine() is used for lines and polygons and has     */
/*      lots of logic to handle horizon crossing.                       */
/************************************************************************/

int msProjectLineEx(reprojectionObj* reprojector, lineObj *line)
{
  int be_careful = reprojector->out->proj != NULL &&
                 msProjIsGeographicCRS(reprojector->out)
                 && !msProjIsGeographicCRS(reprojector->in);

  if( be_careful ) {
    pointObj  startPoint, thisPoint; /* locations in projected space */

    startPoint = line->point[0];

    for(int i=0; i<line->numpoints; i++) {
      double  dist;

      thisPoint = line->point[i];

      /*
      ** Read comments before msTestNeedWrap() to better understand
      ** this dateline wrapping logic.
      */
      msProjectPointEx(reprojector, &(line->point[i]));
      if( i > 0 ) {
        dist = line->point[i].x - line->point[0].x;
        if( fabs(dist) > 180.0 ) {
          if( msTestNeedWrap( thisPoint, startPoint,
                              line->point[0], reprojector ) ) {
            if( dist > 0.0 ) {
              line->point[i].x -= 360.0;
            } else if( dist < 0.0 ) {
              line->point[i].x += 360.0;
            }
          }
        }

      }
    }
  } else {
    for(int i=0; i<line->numpoints; i++) {
      if( msProjectPointEx(reprojector, &(line->point[i])) == MS_FAILURE )
        return MS_FAILURE;
    }
  }

  return(MS_SUCCESS);
}

/************************************************************************/
/*                           msProjectRectGrid()                        */
/************************************************************************/

#define NUMBER_OF_SAMPLE_POINTS 100

static
int msProjectRectGrid(reprojectionObj* reprojector, rectObj *rect)
{
  pointObj prj_point;
  rectObj prj_rect;
  int     failure=0;
  int     ix, iy;

  double dx, dy;
  double x, y;

  prj_rect.minx = DBL_MAX;
  prj_rect.miny = DBL_MAX;
  prj_rect.maxx = -DBL_MAX;
  prj_rect.maxy = -DBL_MAX;

  dx = (rect->maxx - rect->minx)/NUMBER_OF_SAMPLE_POINTS;
  dy = (rect->maxy - rect->miny)/NUMBER_OF_SAMPLE_POINTS;

  /* first ensure the top left corner is processed, even if the rect
     turns out to be degenerate. */

  prj_point.x = rect->minx;
  prj_point.y = rect->miny;
  prj_point.z = 0.0;
  prj_point.m = 0.0;

  msProjectGrowRect(reprojector,&prj_rect,&prj_point,
                    &failure);

  failure = 0;
  for(ix = 0; ix <= NUMBER_OF_SAMPLE_POINTS; ix++ ) {
    x = rect->minx + ix * dx;

    for(iy = 0; iy <= NUMBER_OF_SAMPLE_POINTS; iy++ ) {
      y = rect->miny + iy * dy;

      prj_point.x = x;
      prj_point.y = y;
      msProjectGrowRect(reprojector,&prj_rect,&prj_point,
                        &failure);
    }
  }

  if( prj_rect.minx > prj_rect.maxx ) {
    rect->minx = 0;
    rect->maxx = 0;
    rect->miny = 0;
    rect->maxy = 0;

    msSetError(MS_PROJERR, "All points failed to reproject.", "msProjectRect()");
    return MS_FAILURE;
  }

  if( failure ) {
      msDebug( "msProjectRect(): some points failed to reproject, doing internal sampling.\n" );
  }

  rect->minx = prj_rect.minx;
  rect->miny = prj_rect.miny;
  rect->maxx = prj_rect.maxx;
  rect->maxy = prj_rect.maxy;

  return(MS_SUCCESS);
}

/************************************************************************/
/*                       msProjectRectAsPolygon()                       */
/************************************************************************/

static int
msProjectRectAsPolygon(reprojectionObj* reprojector, rectObj *rect)
{
  shapeObj polygonObj;
  lineObj  ring;
  /*  pointObj ringPoints[NUMBER_OF_SAMPLE_POINTS*4+4]; */
  pointObj *ringPoints;
  int     ix, iy, ixy;

  double dx, dy;

  /* If projecting from longlat to projected */
  /* This hack was introduced for WFS 2.0 compliance testing, but is far */
  /* from being perfect */
  if( reprojector->out && !msProjIsGeographicCRS(reprojector->out) &&
      reprojector->in && msProjIsGeographicCRS(reprojector->in) &&
      fabs(rect->minx - -180.0) < 1e-5 && fabs(rect->miny - -90.0) < 1e-5 &&
      fabs(rect->maxx - 180.0) < 1e-5 && fabs(rect->maxy - 90.0) < 1e-5) {
    pointObj pointTest;
    pointTest.x = -180;
    pointTest.y = 85;
    msProjectPointEx(reprojector, &pointTest);
    /* Detect if we are reprojecting from EPSG:4326 to EPSG:3857 */
    /* and if so use more plausible bounds to avoid issues with computed */
    /* resolution for WCS */
    if (fabs(pointTest.x - -20037508.3427892) < 1e-5 && fabs(pointTest.y - 19971868.8804086) )
    {
        rect->minx = -20037508.3427892;
        rect->miny = -20037508.3427892;
        rect->maxx = 20037508.3427892;
        rect->maxy = 20037508.3427892;
    }
    else
    {
        rect->minx = -1e15;
        rect->miny = -1e15;
        rect->maxx = 1e15;
        rect->maxy = 1e15;
    }
    return MS_SUCCESS;
  }

  /* -------------------------------------------------------------------- */
  /*      Build polygon as steps around the source rectangle              */
  /*      and possibly its diagonal.                                      */
  /* -------------------------------------------------------------------- */
  dx = (rect->maxx - rect->minx)/NUMBER_OF_SAMPLE_POINTS;
  dy = (rect->maxy - rect->miny)/NUMBER_OF_SAMPLE_POINTS;

  if(dx==0 && dy==0) {
    pointObj foo;
    msDebug( "msProjectRect(): Warning: degenerate rect {%f,%f,%f,%f}\n",rect->minx,rect->miny,rect->minx,rect->miny );
    foo.x = rect->minx;
    foo.y = rect->miny;
    msProjectPointEx(reprojector,&foo);
    rect->minx=rect->maxx=foo.x;
    rect->miny=rect->maxy=foo.y;
    return MS_SUCCESS;
  }
  
  /* If there is more than two sample points we will also get samples from the diagonal line */
  ringPoints = (pointObj*) calloc(sizeof(pointObj),NUMBER_OF_SAMPLE_POINTS*5+3);
  ring.point = ringPoints;
  ring.numpoints = 0;

  msInitShape( &polygonObj );
  polygonObj.type = MS_SHAPE_POLYGON;

  /* sample along top */
  if(dx != 0) {
    for(ix = 0; ix <= NUMBER_OF_SAMPLE_POINTS; ix++ ) {
      ringPoints[ring.numpoints].x = rect->minx + ix * dx;
      ringPoints[ring.numpoints++].y = rect->miny;
    }
  }

  /* sample along right side */
  if(dy != 0) {
    for(iy = 1; iy <= NUMBER_OF_SAMPLE_POINTS; iy++ ) {
      ringPoints[ring.numpoints].x = rect->maxx;
      ringPoints[ring.numpoints++].y = rect->miny + iy * dy;
    }
  }

  /* sample along bottom */
  if(dx != 0) {
    for(ix = NUMBER_OF_SAMPLE_POINTS-1; ix >= 0; ix-- ) {
      ringPoints[ring.numpoints].x = rect->minx + ix * dx;
      ringPoints[ring.numpoints++].y = rect->maxy;
    }
  }

  /* sample along left side */
  if(dy != 0) {
    for(iy = NUMBER_OF_SAMPLE_POINTS-1; iy >= 0; iy-- ) {
      ringPoints[ring.numpoints].x = rect->minx;
      ringPoints[ring.numpoints++].y = rect->miny + iy * dy;
    }
  }

  /* sample along diagonal line */
  /* This is done to handle cases where reprojection from world covering projection to one */
  /* which isn't could cause min and max values of the projected rectangle to be invalid */
  if(dy != 0 && dx != 0) {
    /* No need to compute corners as they've already been computed */
    for(ixy = NUMBER_OF_SAMPLE_POINTS-2; ixy >= 1; ixy-- ) {
      ringPoints[ring.numpoints].x = rect->minx + ixy * dx;
      ringPoints[ring.numpoints++].y = rect->miny + ixy * dy;
    }
  }

  msAddLineDirectly( &polygonObj, &ring );

#ifdef notdef
  FILE *wkt = fopen("/tmp/www-before.wkt","w");
  char *tmp = msShapeToWKT(&polygonObj);
  fprintf(wkt,"%s\n", tmp);
  free(tmp);
  fclose(wkt);
#endif

  /* -------------------------------------------------------------------- */
  /*      Attempt to reproject.                                           */
  /* -------------------------------------------------------------------- */
  msProjectShapeLine( reprojector, &polygonObj, 0 );

  /* If no points reprojected, try a grid sampling */
  if( polygonObj.numlines == 0 || polygonObj.line[0].numpoints == 0 ) {
    msFreeShape( &polygonObj );
    return msProjectRectGrid( reprojector, rect );
  }

#ifdef notdef
  wkt = fopen("/tmp/www-after.wkt","w");
  tmp = msShapeToWKT(&polygonObj);
  fprintf(wkt,"%s\n", tmp);
  free(tmp);
  fclose(wkt);
#endif

  /* -------------------------------------------------------------------- */
  /*      Collect bounds.                                                 */
  /* -------------------------------------------------------------------- */
  rect->minx = rect->maxx = polygonObj.line[0].point[0].x;
  rect->miny = rect->maxy = polygonObj.line[0].point[0].y;

  for( ix = 1; ix < polygonObj.line[0].numpoints; ix++ ) {
    pointObj  *pnt = polygonObj.line[0].point + ix;

    rect->minx = MS_MIN(rect->minx,pnt->x);
    rect->maxx = MS_MAX(rect->maxx,pnt->x);
    rect->miny = MS_MIN(rect->miny,pnt->y);
    rect->maxy = MS_MAX(rect->maxy,pnt->y);
  }

  msFreeShape( &polygonObj );

  /* -------------------------------------------------------------------- */
  /*      Special case to handle reprojection from "more than the         */
  /*      whole world" projected coordinates that sometimes produce a     */
  /*      region greater than 360 degrees wide due to various wrapping    */
  /*      logic.                                                          */
  /* -------------------------------------------------------------------- */
  if( reprojector->out && msProjIsGeographicCRS(reprojector->out) &&
      reprojector->in && !msProjIsGeographicCRS(reprojector->in)
      && rect->maxx - rect->minx > 360.0 &&
      !reprojector->out->gt.need_geotransform ) {
    rect->maxx = 180;
    rect->minx = -180;
  }

  return MS_SUCCESS;
}

/************************************************************************/
/*                        msProjectHasLonWrap()                         */
/************************************************************************/

int msProjectHasLonWrap(projectionObj *in, double* pdfLonWrap)
{
    int i;
    if( pdfLonWrap )
        *pdfLonWrap = 0;

    if( !msProjIsGeographicCRS(in) )
        return MS_FALSE;

    for( i = 0; i < in->numargs; i++ )
    {
        if( strncmp(in->args[i], "lon_wrap=",
                    strlen("lon_wrap=")) == 0 )
        {
            if( pdfLonWrap )
                *pdfLonWrap = atof(in->args[i] + strlen("lon_wrap="));
            return MS_TRUE;
        }
    }
    return MS_FALSE;
}

/************************************************************************/
/*                           msProjectRect()                            */
/************************************************************************/

int msProjectRect(projectionObj *in, projectionObj *out, rectObj *rect)
{
  char *over = "+over";
  int ret;
  int bFreeInOver = MS_FALSE;
  int bFreeOutOver = MS_FALSE;
  projectionObj in_over,out_over,*inp,*outp;
  double dfLonWrap = 0.0;
  reprojectionObj* reprojector = NULL;

  /* Detect projecting from polar stereographic to longlat */
  if( in && !in->gt.need_geotransform &&
      out && !out->gt.need_geotransform &&
      !msProjIsGeographicCRS(in) && msProjIsGeographicCRS(out) )
  {
      reprojector = msProjectCreateReprojector(in, out);
      for( int sign = 1; sign >= -1; sign -= 2 )
      {
          pointObj p;
          p.x = 0.0;
          p.y = 0.0;
          if( reprojector &&
              msProjectPointEx(reprojector, &p) == MS_SUCCESS &&
              fabs(p.y - sign * 90) < 1e-8 )
          {
            /* Is the pole in the rectangle ? */
            if( 0 >= rect->minx && 0 >= rect->miny &&
                0 <= rect->maxx && 0 <= rect->maxy )
            {
                if( msProjectRectAsPolygon(reprojector, rect ) == MS_SUCCESS )
                {
                    rect->minx = -180.0;
                    rect->maxx = 180.0;
                    rect->maxy = sign * 90.0;
                    msProjectDestroyReprojector(reprojector);
                    return MS_SUCCESS;
                }
            }
            /* Are we sure the dateline is not enclosed ? */
            else if( rect->maxy < 0 || rect->maxx < 0 || rect->minx > 0 )
            {
                ret = msProjectRectAsPolygon(reprojector, rect );
                msProjectDestroyReprojector(reprojector);
                return ret;
            }
          }
      }
  }

  /* Detect projecting from polar stereographic to another projected system */
  else if( in && !in->gt.need_geotransform &&
      out && !out->gt.need_geotransform &&
      !msProjIsGeographicCRS(in) && !msProjIsGeographicCRS(out) )
  {
      reprojectionObj* reprojectorToLongLat = msProjectCreateReprojector(in, NULL);
      for( int sign = 1; sign >= -1; sign -= 2 )
      {
          pointObj p;
          p.x = 0.0;
          p.y = 0.0;
          if( reprojectorToLongLat &&
              msProjectPointEx(reprojectorToLongLat, &p) == MS_SUCCESS &&
              fabs(p.y - 90) < 1e-8 )
          {
            reprojector = msProjectCreateReprojector(in, out);
            /* Is the pole in the rectangle ? */
            if( 0 >= rect->minx && 0 >= rect->miny &&
                0 <= rect->maxx && 0 <= rect->maxy )
            {
                if( msProjectRectAsPolygon(reprojector, rect ) == MS_SUCCESS )
                {
                    msProjectDestroyReprojector(reprojector);
                    msProjectDestroyReprojector(reprojectorToLongLat);
                    return MS_SUCCESS;
                }
            }
            /* Are we sure the dateline is not enclosed ? */
            else if( rect->maxy < 0 || rect->maxx < 0 || rect->minx > 0 )
            {
                ret = msProjectRectAsPolygon(reprojector, rect );
                msProjectDestroyReprojector(reprojector);
                msProjectDestroyReprojector(reprojectorToLongLat);
                return ret;
            }
          }
      }
      msProjectDestroyReprojector(reprojectorToLongLat);
  }


  if(in && msProjectHasLonWrap(in, &dfLonWrap) && dfLonWrap == 180.0) {
    inp = in;
    outp = out;
    if( rect->maxx > 180.0 ) {
      rect->minx = -180.0;
      rect->maxx = 180.0;
    }
  }
  /* 
   * Issue #4892: When projecting a rectangle we do not want proj to wrap resulting
   * coordinates around the dateline, as in practice a requested bounding box of
   * -22.000.000, -YYY, 22.000.000, YYY should be projected as
   * -190,-YYY,190,YYY rather than 170,-YYY,-170,YYY as would happen when wrapping (and
   *  vice-versa when projecting from lonlat to metric).
   *  To enforce this, we clone the input projections and add the "+over" proj 
   *  parameter in order to disable dateline wrapping.
   */ 
  else {
    int apply_over = MS_TRUE;
#if PROJ_VERSION_MAJOR >= 6 && PROJ_VERSION_MAJOR < 9
    // Workaround PROJ [6,9[ bug (fixed per https://github.com/OSGeo/PROJ/pull/3055)
    // that prevents datum shifts from being applied when +over is added to +init=epsg:XXXX
    // This is far from being bullet proof but it should work for most common use cases
    if(in && in->proj)
    {
        if( in->numargs == 1 && EQUAL(in->args[0], "init=epsg:4326") &&
            rect->minx >= -180 && rect->maxx <= 180 )
        {
            apply_over = MS_FALSE;
        }
        else if( in->numargs == 1 && EQUAL(in->args[0], "init=epsg:3857") &&
                 rect->minx >= -20037508.3427892 && rect->maxx <= 20037508.3427892 )
        {
            apply_over = MS_FALSE;
        }
    }
#endif
    if(out && apply_over && out->numargs > 0 &&
        (strncmp(out->args[0], "init=", 5) == 0 || strncmp(out->args[0], "proj=", 5) == 0)) {
      bFreeOutOver = MS_TRUE;
      msInitProjection(&out_over);
      msCopyProjectionExtended(&out_over,out,&over,1);
      outp = &out_over;
      if( reprojector ) {
          msProjectDestroyReprojector(reprojector);
          reprojector = NULL;
      }
    } else {
      outp = out;
    }
    if(in && apply_over && in->numargs > 0 &&
        (strncmp(in->args[0], "init=", 5) == 0 || strncmp(in->args[0], "proj=", 5) == 0)) {
      bFreeInOver = MS_TRUE;
      msInitProjection(&in_over);
      msCopyProjectionExtended(&in_over,in,&over,1);
      inp = &in_over;
      /* coverity[dead_error_begin] */
      if( reprojector ) {
          msProjectDestroyReprojector(reprojector);
          reprojector = NULL;
      }
    } else {
      inp = in;
    }
  }
  if( reprojector == NULL )
  {
     reprojector = msProjectCreateReprojector(inp, outp);
  }
  ret = reprojector ? msProjectRectAsPolygon(reprojector, rect ) : MS_FAILURE;
  msProjectDestroyReprojector(reprojector);
  if(bFreeInOver)
    msFreeProjection(&in_over);
  if(bFreeOutOver)
    msFreeProjection(&out_over);
  return ret;
}

#if PROJ_VERSION_MAJOR < 6

static int msProjectSortString(const void* firstelt, const void* secondelt)
{
    char* firststr = *(char**)firstelt;
    char* secondstr = *(char**)secondelt;
    return strcmp(firststr, secondstr);
}

/************************************************************************/
/*                        msGetProjectNormalized()                      */
/************************************************************************/

static projectionObj* msGetProjectNormalized( const projectionObj* p )
{
  int i;
#if PROJ_VERSION_MAJOR >= 6
  const char* pszNewProj4Def;
#else
  char* pszNewProj4Def;
#endif
  projectionObj* pnew;

  pnew = (projectionObj*)msSmallMalloc(sizeof(projectionObj));
  msInitProjection(pnew);
  msCopyProjection(pnew, (projectionObj*)p);

  if(p->proj == NULL )
      return pnew;

  /* Normalize definition so that msProjectDiffers() works better */
#if PROJ_VERSION_MAJOR >= 6
  pszNewProj4Def = proj_as_proj_string(p->proj_ctx->proj_ctx, p->proj, PJ_PROJ_4, NULL);
#else
  pszNewProj4Def = pj_get_def( p->proj, 0 );
#endif
  msFreeCharArray(pnew->args, pnew->numargs);
  pnew->args = msStringSplit(pszNewProj4Def,'+', &pnew->numargs);
  for(i = 0; i < pnew->numargs; i++)
  {
      /* Remove trailing space */
      if( strlen(pnew->args[i]) > 0 && pnew->args[i][strlen(pnew->args[i])-1] == ' ' )
          pnew->args[i][strlen(pnew->args[i])-1] = '\0';
      /* Remove spurious no_defs or init= */
      if( strcmp(pnew->args[i], "no_defs") == 0 ||
          strncmp(pnew->args[i], "init=", 5) == 0 )
      {
          if( i < pnew->numargs - 1 )
          {
              msFree(pnew->args[i]);
              memmove(pnew->args + i, pnew->args + i + 1,
                      sizeof(char*) * (pnew->numargs - 1 -i ));
          }
          else 
          {
              msFree(pnew->args[i]);
          }
          pnew->numargs --;
          i --;
          continue;
      }
  }
  /* Sort the strings so they can be compared */
  qsort(pnew->args, pnew->numargs, sizeof(char*), msProjectSortString);
  /*{
      fprintf(stderr, "'%s' =\n", pszNewProj4Def);
      for(i = 0; i < p->numargs; i++)
          fprintf(stderr, "'%s' ", p->args[i]);
      fprintf(stderr, "\n");
  }*/
#if PROJ_VERSION_MAJOR < 6
  pj_dalloc(pszNewProj4Def);
#endif

  return pnew;
}
#endif

/************************************************************************/
/*                        msProjectionsDiffer()                         */
/************************************************************************/

/*
** Compare two projections, and return MS_TRUE if they differ.
**
** For now this is implemented by exact comparison of the projection
** arguments, but eventually this should call a PROJ.4 function with
** more awareness of the issues.
**
** NOTE: MS_FALSE is returned if either of the projection objects
** has no arguments, since reprojection won't work anyways.
*/

static int msProjectionsDifferInternal( projectionObj *proj1, projectionObj *proj2 )

{
  int   i;

  if( proj1->numargs == 0 || proj2->numargs == 0 )
    return MS_FALSE;

  if( proj1->numargs != proj2->numargs )
    return MS_TRUE;

  /* This test should be more rigerous. */
  if( proj1->gt.need_geotransform
      || proj2->gt.need_geotransform )
    return MS_TRUE;

  for( i = 0; i < proj1->numargs; i++ ) {
    if( strcmp(proj1->args[i],proj2->args[i]) != 0 )
      return MS_TRUE;
  }

  return MS_FALSE;
}

int msProjectionsDiffer( projectionObj *proj1, projectionObj *proj2 )
{
    int ret;

    ret = msProjectionsDifferInternal(proj1, proj2);
#if PROJ_VERSION_MAJOR < 6
    if( ret &&
        /* to speed up things, do normalization only if one proj is */
        /* likely of the form init=epsg:XXX and the other proj=XXX datum=YYY... */
        ( (proj1->numargs == 1 && proj2->numargs > 1) ||
          (proj1->numargs > 1 && proj2->numargs == 1) ) )
    {
        projectionObj* p1normalized;
        projectionObj* p2normalized;

        p1normalized = msGetProjectNormalized( proj1 );
        p2normalized = msGetProjectNormalized( proj2 );
        ret = msProjectionsDifferInternal(p1normalized, p2normalized);
        msFreeProjection(p1normalized);
        msFree(p1normalized);
        msFreeProjection(p2normalized);
        msFree(p2normalized);
    }
#endif
    return ret;
}

/************************************************************************/
/*                           msTestNeedWrap()                           */
/************************************************************************/
/*

Frank Warmerdam, Nov, 2001.

See Also:

http://mapserver.gis.umn.edu/bugs/show_bug.cgi?id=15

Proposal:

Modify msProjectLine() so that it "dateline wraps" objects when necessary
in order to preserve their shape when reprojecting to lat/long.  This
will be accomplished by:

1) As each vertex is reprojected, compare the X distance between that
   vertex and the previous vertex.  If it is less than 180 then proceed to
   the next vertex without any special logic, otherwise:

2) Reproject the center point of the line segment from the last vertex to
   the current vertex into lat/long.  Does it's longitude lie between the
   longitudes of the start and end point.  If yes, return to step 1) for
   the next vertex ... everything is fine.

3) We have determined that this line segment is suffering from 360 degree
   wrap to keep in the -180 to +180 range.  Now add or subtract 360 degrees
   as determined from the original sign of the distances.

This is similar to the code there now (though disabled in CVS); however,
it will ensure that big boxes will remain big, and not get dateline wrapped
because of the extra test in step 2).  However step 2 is invoked only very
rarely so this process takes little more than the normal process.  In fact,
if we were sufficiently concerned about performance we could do a test on the
shape MBR in lat/long space, and if the width is less than 180 we know we never
need to even do test 1).

What doesn't this resolve:

This ensures that individual lines are kept in the proper shape when
reprojected to geographic space.  However, it does not:

 o Ensure that all rings of a polygon will get transformed to the same "side"
   of the world.  Depending on starting points of the different rings it is
   entirely possible for one ring to end up in the -180 area and another ring
   from the same polygon to end up in the +180 area.  We might possibly be
   able to achieve this though, by treating the multi-ring polygon as a whole
   and testing the first point of each additional ring against the last
   vertex of the previous ring (or any previous vertex for that matter).

 o It does not address the need to cut up lines and polygons into distinct
   chunks to preserve the correct semantics.  Really a polygon that
   spaces the dateline in a -180 to 180 view should get split into two
   polygons.  We haven't addressed that, though if it were to be addressed,
   it could be done as a followon and distinct step from what we are doing
   above.  In fact this sort of improvement (split polygons based on dateline
   or view window) should be done for all lat/long shapes regardless of
   whether they are being reprojected from another projection.

 o It does not address issues related to viewing rectangles that go outside
   the -180 to 180 longitude range.  For instance, it is entirely reasonable
   to want a 160 to 200 longitude view to see an area on the dateline clearly.
   Currently shapes in the -180 to -160 range which should be displayed in the
   180 to 200 portion of that view will not be because there is no recogition
   that they belong there.


*/

static int msTestNeedWrap( pointObj pt1, pointObj pt2, pointObj pt2_geo,
                           reprojectionObj* reprojector )

{
  pointObj  middle;
  projectionObj* out = reprojector->out;

  middle.x = (pt1.x + pt2.x) * 0.5;
  middle.y = (pt1.y + pt2.y) * 0.5;

  if( msProjectPointEx( reprojector, &pt1 ) == MS_FAILURE
      || msProjectPointEx( reprojector, &pt2 ) == MS_FAILURE
      || msProjectPointEx( reprojector, &middle ) == MS_FAILURE )
    return 0;

  /*
   * If the last point was moved, then we are considered due for a
   * move to.
   */
  if( out->gt.need_geotransform && out->gt.geotransform[2] == 0 ) {
    if( fabs( (pt2_geo.x-pt2.x) * out->gt.geotransform[1] ) > 180.0 )
      return 1;
  } else {
    if( fabs(pt2_geo.x-pt2.x) > 180.0 )
      return 1;
  }

  /*
   * Otherwise, test to see if the middle point transforms
   * to be between the end points. If yes, no wrapping is needed.
   * Otherwise wrapping is needed.
   */
  if( (middle.x < pt1.x && middle.x < pt2_geo.x)
      || (middle.x > pt1.x && middle.x > pt2_geo.x) )
    return 1;
  else
    return 0;
}

/************************************************************************/
/*                            msProjFinder()                            */
/************************************************************************/

#if PROJ_VERSION_MAJOR < 6
static char *last_filename = NULL;

static const char *msProjFinder( const char *filename)

{
  if( last_filename != NULL )
    free( last_filename );

  if( filename == NULL )
    return NULL;

  if( ms_proj_lib == NULL )
    return filename;

  last_filename = (char *) malloc(strlen(filename)+strlen(ms_proj_lib)+2);
  sprintf( last_filename, "%s/%s", ms_proj_lib, filename );

  return last_filename;
}
#endif

/************************************************************************/
/*                       msProjLibInitFromEnv()                         */
/************************************************************************/
void msProjLibInitFromEnv()
{
  const char *val;

  if( (val=CPLGetConfigOption( "PROJ_LIB", NULL )) != NULL ) {
    msSetPROJ_LIB(val, NULL);
  }
}

/************************************************************************/
/*                           msSetPROJ_LIB()                            */
/************************************************************************/
void msSetPROJ_LIB( const char *proj_lib, const char *pszRelToPath )

{
  char *extended_path = NULL;

  /* Handle relative path if applicable */
  if( proj_lib && pszRelToPath
      && proj_lib[0] != '/'
      && proj_lib[0] != '\\'
      && !(proj_lib[0] != '\0' && proj_lib[1] == ':') ) {
    struct stat stat_buf;
    extended_path = (char*) msSmallMalloc(strlen(pszRelToPath)
                                          + strlen(proj_lib) + 10);
    sprintf( extended_path, "%s/%s", pszRelToPath, proj_lib );

#ifndef S_ISDIR
#  define S_ISDIR(x) ((x) & S_IFDIR)
#endif

    if( stat( extended_path, &stat_buf ) == 0
        && S_ISDIR(stat_buf.st_mode) )
      proj_lib = extended_path;
  }


  msAcquireLock( TLOCK_PROJ );
#if PROJ_VERSION_MAJOR >= 6
  if( proj_lib == NULL && ms_proj_lib == NULL )
  {
     /* do nothing */
  }
  else if( proj_lib != NULL && ms_proj_lib != NULL &&
           strcmp(proj_lib, ms_proj_lib) == 0 )
  {
    /* do nothing */
  }
  else
  {
    ms_proj_lib_change_counter++;
    free( ms_proj_lib );
    ms_proj_lib = proj_lib ? msStrdup(proj_lib) : NULL;
  }
#else
  {
    static int finder_installed = 0;
    if( finder_installed == 0 && proj_lib != NULL) {
      finder_installed = 1;
      pj_set_finder( msProjFinder );
    }
  }

  if (proj_lib == NULL) pj_set_finder(NULL);

  if( ms_proj_lib != NULL ) {
    free( ms_proj_lib );
    ms_proj_lib = NULL;
  }

  if( last_filename != NULL ) {
    free( last_filename );
    last_filename = NULL;
  }

  if( proj_lib != NULL )
    ms_proj_lib = msStrdup( proj_lib );
#endif
  msReleaseLock( TLOCK_PROJ );

#if GDAL_VERSION_MAJOR >= 3
  if( ms_proj_lib != NULL )
  {
    const char* const apszPaths[] = { ms_proj_lib, NULL };
    OSRSetPROJSearchPaths(apszPaths);
  }
#endif

  if ( extended_path )
    msFree( extended_path );
}

/************************************************************************/
/*                       msGetProjectionString()                        */
/*                                                                      */
/*      Return the projection string.                                   */
/************************************************************************/

char *msGetProjectionString(projectionObj *proj)
{
  char        *pszProjString = NULL;
  int         nLen = 0;

  if (proj) {
    /* -------------------------------------------------------------------- */
    /*      Alloc buffer large enough to hold the whole projection defn     */
    /* -------------------------------------------------------------------- */
    for (int i=0; i<proj->numargs; i++) {
      if (proj->args[i])
        nLen += (strlen(proj->args[i]) + 2);
    }

    pszProjString = (char*)malloc(sizeof(char) * nLen+1);
    pszProjString[0] = '\0';

    /* -------------------------------------------------------------------- */
    /*      Plug each arg into the string with a '+' prefix                 */
    /* -------------------------------------------------------------------- */
    for (int i=0; i<proj->numargs; i++) {
      if (!proj->args[i] || strlen(proj->args[i]) == 0)
        continue;
      if (pszProjString[0] == '\0') {
        /* no space at beginning of line */
        if (proj->args[i][0] != '+')
          strcat(pszProjString, "+");
      } else {
        if (proj->args[i][0] != '+')
          strcat(pszProjString, " +");
        else
          strcat(pszProjString, " ");
      }
      strcat(pszProjString, proj->args[i]);
    }
  }

  return pszProjString;
}

/************************************************************************/
/*                       msIsAxisInvertedProj()                         */
/*                                                                      */
/*      Return MS_TRUE is the proj object has epgsaxis=ne               */
/************************************************************************/

int msIsAxisInvertedProj( projectionObj *proj )
{
  int i;
  const char *axis = NULL;

  for( i = 0; i < proj->numargs; i++ ) {
    if( strstr(proj->args[i],"epsgaxis=") != NULL ) {
      axis = strstr(proj->args[i],"=") + 1;
      break;
    }
  }

  if( axis == NULL )
    return MS_FALSE;

  if( strcasecmp(axis,"en") == 0 )
    return MS_FALSE;

  if( strcasecmp(axis,"ne") != 0 ) {
    msDebug( "msIsAxisInvertedProj(): odd +epsgaxis= value: '%s'.",
             axis );
    return MS_FALSE;
  }
  
  return MS_TRUE;
}

/************************************************************************/
/*                       msAxisNormalizePoints()                        */
/*                                                                      */
/*      Convert the passed points to "easting, northing" axis           */
/*      orientation if they are not already.                            */
/************************************************************************/

void msAxisNormalizePoints( projectionObj *proj, int count,
                            double *x, double *y )

{
  int i;
  if( !msIsAxisInvertedProj(proj ) )
      return;

  /* Switch axes */
  for( i = 0; i < count; i++ ) {
    double tmp;

    tmp = x[i];
    x[i] = y[i];
    y[i] = tmp;
  }
}



/************************************************************************/
/*                             msAxisSwapShape                          */
/*                                                                      */
/*      Utility function to swap x and y coordiatesn Use for now for    */
/*      WFS 1.1.x                                                       */
/************************************************************************/
void msAxisSwapShape(shapeObj *shape)
{
  double tmp;
  int i,j;

  if (shape) {
    for(i=0; i<shape->numlines; i++) {
      for( j=0; j<shape->line[i].numpoints; j++ ) {
        tmp = shape->line[i].point[j].x;
        shape->line[i].point[j].x = shape->line[i].point[j].y;
        shape->line[i].point[j].y = tmp;
      }
    }

    /*swap bounds*/
    tmp = shape->bounds.minx;
    shape->bounds.minx = shape->bounds.miny;
    shape->bounds.miny = tmp;

    tmp = shape->bounds.maxx;
    shape->bounds.maxx = shape->bounds.maxy;
    shape->bounds.maxy = tmp;
  }
}

/************************************************************************/
/*                      msAxisDenormalizePoints()                       */
/*                                                                      */
/*      Convert points from easting,northing orientation to the         */
/*      preferred epsg orientation of this projectionObj.               */
/************************************************************************/

void msAxisDenormalizePoints( projectionObj *proj, int count,
                              double *x, double *y )

{
  /* For how this is essentially identical to normalizing */
  msAxisNormalizePoints( proj, count, x, y );
}

/************************************************************************/
/*                        msProjIsGeographicCRS()                       */
/*                                                                      */
/*      Returns whether a CRS is a geographic one.                      */
/************************************************************************/

int msProjIsGeographicCRS(projectionObj* proj)
{
#if PROJ_VERSION_MAJOR >= 6
    PJ_TYPE type;
    if( !proj->proj )
        return FALSE;
    type = proj_get_type(proj->proj);
    if( type == PJ_TYPE_GEOGRAPHIC_2D_CRS || type == PJ_TYPE_GEOGRAPHIC_3D_CRS )
        return TRUE;
    if( type == PJ_TYPE_BOUND_CRS )
    {
        PJ* base_crs = proj_get_source_crs(proj->proj_ctx->proj_ctx, proj->proj);
        type = proj_get_type(base_crs);
        proj_destroy(base_crs);
        return type == PJ_TYPE_GEOGRAPHIC_2D_CRS || type == PJ_TYPE_GEOGRAPHIC_3D_CRS;
    }
    return FALSE;
#else
    return proj->proj != NULL && pj_is_latlong(proj->proj);
#endif
}

/************************************************************************/
/*                        ConvertProjUnitStringToMS                     */
/*                                                                      */
/*       Returns mapserver's unit code corresponding to the proj        */
/*      unit passed as argument.                                        */
/*       Please refer to ./src/pj_units.c file in the Proj.4 module.    */
/************************************************************************/
static int ConvertProjUnitStringToMS(const char *pszProjUnit)
{
  if (strcmp(pszProjUnit, "m") ==0) {
    return MS_METERS;
  } else if (strcmp(pszProjUnit, "km") ==0) {
    return MS_KILOMETERS;
  } else if (strcmp(pszProjUnit, "mi") ==0 || strcmp(pszProjUnit, "us-mi") ==0) {
    return MS_MILES;
  } else if (strcmp(pszProjUnit, "in") ==0 || strcmp(pszProjUnit, "us-in") ==0 ) {
    return MS_INCHES;
  } else if (strcmp(pszProjUnit, "ft") ==0 || strcmp(pszProjUnit, "us-ft") ==0) {
    return MS_FEET;
  } else if (strcmp(pszProjUnit, "kmi") == 0) {
    return MS_NAUTICALMILES;
  }

  return -1;
}

/************************************************************************/
/*           int GetMapserverUnitUsingProj(projectionObj *psProj)       */
/*                                                                      */
/*      Return a mapserver unit corresponding to the projection         */
/*      passed. Retunr -1 on failure                                    */
/************************************************************************/
int GetMapserverUnitUsingProj(projectionObj *psProj)
{
#if PROJ_VERSION_MAJOR >= 6
  const char *proj_str;
#else
  char *proj_str;
#endif

  if( msProjIsGeographicCRS( psProj ) )
    return MS_DD;

#if PROJ_VERSION_MAJOR >= 6
  proj_str = proj_as_proj_string(psProj->proj_ctx->proj_ctx, psProj->proj, PJ_PROJ_4, NULL);
#else
  proj_str = pj_get_def( psProj->proj, 0 );
#endif
  if( proj_str == NULL )
    return -1;

  /* -------------------------------------------------------------------- */
  /*      Handle case of named units.                                     */
  /* -------------------------------------------------------------------- */
  if( strstr(proj_str,"units=") != NULL ) {
    char units[32];
    char *blank;

    strlcpy( units, (strstr(proj_str,"units=")+6), sizeof(units) );
#if PROJ_VERSION_MAJOR < 6
    pj_dalloc( proj_str );
#endif

    blank = strchr(units, ' ');
    if( blank != NULL )
      *blank = '\0';

    return ConvertProjUnitStringToMS( units );
  }

  /* -------------------------------------------------------------------- */
  /*      Handle case of to_meter value.                                  */
  /* -------------------------------------------------------------------- */
  if( strstr(proj_str,"to_meter=") != NULL ) {
    char to_meter_str[32];
    char *blank;
    double to_meter;

    strlcpy(to_meter_str,(strstr(proj_str,"to_meter=")+9),
            sizeof(to_meter_str));
#if PROJ_VERSION_MAJOR < 6
    pj_dalloc( proj_str );
#endif

    blank = strchr(to_meter_str, ' ');
    if( blank != NULL )
      *blank = '\0';

    to_meter = atof(to_meter_str);

    if( fabs(to_meter-1.0) < 0.0000001 )
      return MS_METERS;
    else if( fabs(to_meter-1000.0) < 0.00001 )
      return MS_KILOMETERS;
    else if( fabs(to_meter-0.3048) < 0.0001 )
      return MS_FEET;
    else if( fabs(to_meter-0.0254) < 0.0001 )
      return MS_INCHES;
    else if( fabs(to_meter-1609.344) < 0.001 )
      return MS_MILES;
    else if( fabs(to_meter-1852.0) < 0.1 )
      return MS_NAUTICALMILES;
    else
      return -1;
  }

#if PROJ_VERSION_MAJOR < 6
  pj_dalloc( proj_str );
#endif
  return -1;
}

/************************************************************************/
/*                   msProjectionContextGetFromPool()                   */
/*                                                                      */
/*       Returns a projection context from the pool, or create a new    */
/*       one if the pool is empty.                                      */
/*       After use, it should normally be returned with                 */
/*       msProjectionContextReleaseToPool()                             */
/************************************************************************/

projectionContext* msProjectionContextGetFromPool()
{
    projectionContext* context;
    msAcquireLock( TLOCK_PROJ );

    if( headOfLinkedListOfProjContext )
    {
        LinkedListOfProjContext* next = headOfLinkedListOfProjContext->next;
        context = headOfLinkedListOfProjContext->context;
        msFree(headOfLinkedListOfProjContext);
        headOfLinkedListOfProjContext = next;
    }
    else
    {
        context = msProjectionContextCreate();
    }

    msReleaseLock( TLOCK_PROJ );
    return context;
}

/************************************************************************/
/*                  msProjectionContextReleaseToPool()                  */
/************************************************************************/

void msProjectionContextReleaseToPool(projectionContext* ctx)
{
    LinkedListOfProjContext* link =
        (LinkedListOfProjContext*)msSmallMalloc(sizeof(LinkedListOfProjContext));
    link->context = ctx;
    msAcquireLock( TLOCK_PROJ );
    link->next = headOfLinkedListOfProjContext;
    headOfLinkedListOfProjContext = link;
    msReleaseLock( TLOCK_PROJ );
}

/************************************************************************/
/*                   msProjectionContextPoolCleanup()                   */
/************************************************************************/

void msProjectionContextPoolCleanup()
{
    LinkedListOfProjContext* link;
    msAcquireLock( TLOCK_PROJ );
    link = headOfLinkedListOfProjContext;
    while( link )
    {
        LinkedListOfProjContext* next = link->next;
        msProjectionContextUnref(link->context);
        msFree(link);
        link = next;
    }
    headOfLinkedListOfProjContext = NULL;
    msReleaseLock( TLOCK_PROJ );
}

/**********************************************************************
 * $Id$
 *
 * Name:     php_proj.c
 * Project:  PHP wraper function to PROJ4 projection module
 * Language: ANSI C
 * Purpose:  External interface functions
 * Author:   Assefa Yewondwossen (assefa@dmsolutions.on.ca)
 *
 **********************************************************************
 * Copyright (c) 2000, DM Solutions Group inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************
 *
 * $Log$
 * Revision 1.2  2000/11/08 15:44:16  dan
 * Correct compilation errors with php4.
 *
 * Revision 1.1  2000/11/02 16:39:55  dan
 * PHP PROJ4 module.
 *
 *
 **********************************************************************/

/*
 *         PHP PROJ4 Module
 *
 *  This is a PHP module that gives acces to basic PROJ4 projection 
 *  functionalities.
 *
 * There are four functions available in this module :
 *
 * 1) pj_init : create and initializes a projection structures
 * 
 *    PJ pj_init(array_of_parameters)
 * 
 *    Example : $projarray[0] = "proj=lcc";
 *              $projarray[1] = "ellps=GRS80";
 *              $projarray[2] = "lat_0=49";
 *              $projarray[3] = "lon_0=-95";
 *              $projarray[4] = "lat_1=49";
 *              $projarray[5] = "lat_2=77";
 *
 *              $pj = pj_init($projarray);
 *
 * 2) pj_fwd : Performs a projection from lat/long coordinates to
 *             cartesian coordinates.
 * 
 * retrun_array pj_fwd(double lat, double long, PJ pj)
 *
 *   Example :  $lat = 45.25;
 *              $long = -75.42;
 *
 *              $ret = pj_fwd($ingeox, $ingeoy, $pj);
 *              printf("geo x = %f<br>\n", $ret["u"]);
 *              printf("geo y = %f<br>\n",$ret["v"]);
 *
 * 3) pj_inv : Performs a projection from cartesian coordinates to
 *             lat/long  coordinates .
 * 
 * retrun_array pj_fwd(double geox, double geoy, PJ pj)
 *
 *   Example :  $ingeox = 1537490.335842;
 *              $ingeoy = -181633.471555;
 *
 *              $ret = pj_inv($ingeox, $ingeoy, $pj);
 *              printf("lat = %f<br>\n", $ret["u"]);
 *              printf("lon = %f<br>\n",$ret["v"]);
 *
 * 4) pj_free : frees PJ structure
 *     
 *  void pj_free(PJ pj); 
 *
 **********************************************************************/

#ifdef USE_PROJ
#include <projects.h>
#include "php_mapscript_util.h"

#ifdef PHP4
#include "php.h"
#include "php_globals.h"
#include "ext/standard/php_standard.h"
#include "ext/standard/info.h"
#else
#include "phpdl.h"
#include "php3_list.h"
#include "functions/head.h"   /* php3_header() */
#endif

#include "maperror.h"

#include <time.h>

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <process.h>
#else
#include <errno.h>
#endif

#ifdef PHP4
#define ZEND_DEBUG 0
#endif

#ifndef DLEXPORT 
#define DLEXPORT ZEND_DLEXPORT
#endif

#define PHP_PROJ_VERSION "1.0.000 (Nov. 1, 2000)"

/*=====================================================================
 *                         Prototypes
 *====================================================================*/
DLEXPORT void php_proj_pj_init(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php_proj_pj_fwd(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php_proj_pj_inv(INTERNAL_FUNCTION_PARAMETERS);
DLEXPORT void php_proj_pj_free(INTERNAL_FUNCTION_PARAMETERS);


DLEXPORT void php_info_proj(void);
DLEXPORT int  php_init_proj(INIT_FUNC_ARGS);
DLEXPORT int  php_end_proj(SHUTDOWN_FUNC_ARGS);


#define PHPMS_GLOBAL(a) a
static int le_projobj;

function_entry php_proj_functions[] = {
    {"pj_fwd",  php_proj_pj_fwd,   NULL},
    {"pj_inv",  php_proj_pj_inv,   NULL}, 
    {"pj_init",  php_proj_pj_init,   NULL},
    {"pj_free",  php_proj_pj_free,   NULL},
    {NULL, NULL, NULL}
};


php3_module_entry php_proj_module_entry = {
    "PHPPROJ", php_proj_functions, php_init_proj, php_end_proj,
    NULL, NULL, php_info_proj, STANDARD_MODULE_PROPERTIES 
};


#if COMPILE_DL
DLEXPORT php3_module_entry *get_module(void) 
{ 
    return &php_proj_module_entry; 
}
#endif


DLEXPORT void php_info_proj(void) 
{
    php3_printf(" Version %s<br>\n", PHP_PROJ_VERSION);

    php3_printf("<BR>\n");
}

DLEXPORT int php_init_proj(INIT_FUNC_ARGS)
{
    PHPMS_GLOBAL(le_projobj)  = 
        register_list_destructors(php_proj_pj_free,
                                  NULL);
    return SUCCESS;
}

DLEXPORT int php_end_proj(SHUTDOWN_FUNC_ARGS)
{
    return SUCCESS;
}


#if !defined  DEG_TO_RAD
#define DEG_TO_RAD      0.0174532925199432958
#endif

#if !defined RAD_TO_DEG
#define RAD_TO_DEG	57.29577951308232
#endif

/**********************************************************************
 *                       _php_proj_build_proj_object
 **********************************************************************/
static long _php_proj_build_proj_object(PJ *pj,
                                        HashTable *list, pval *return_value)
{
    int pj_id;

    if (pj == NULL)
        return 0;

    pj_id = php3_list_insert(pj, PHPMS_GLOBAL(le_projobj));

    _phpms_object_init(return_value, pj_id, NULL, NULL);

    return pj_id;
}


/************************************************************************/
/*       DLEXPORT void php_proj_pj_init(INTERNAL_FUNCTION_PARAMETERS)   */
/*                                                                      */
/*      Creates and initialize a  PJ structure that can be used with    */
/*      proj_fwd and proj_inv function.                                 */
/*                                                                      */
/*       Parameter :                                                    */
/*                                                                      */
/*          array : array of parameters                                 */
/*                                                                      */
/*       Ex :                                                           */
/*                                                                      */
/*              $projarray[0] = "proj=lcc";                             */
/*              $projarray[1] = "ellps=GRS80";                          */
/*              $projarray[2] = "lat_0=49";                             */
/*              $projarray[3] = "lon_0=-95";                            */
/*              $projarray[4] = "lat_1=49";                             */
/*              $projarray[5] = "lat_2=77";                             */
/*                                                                      */
/*              $pj = pj_init($projarray);                              */
/*                                                                      */
/************************************************************************/
DLEXPORT void php_proj_pj_init(INTERNAL_FUNCTION_PARAMETERS)
{
    pval        *pArrayOfParams = NULL;

#ifdef PHP4
    pval        **pParam = NULL;
    HashTable   *list=NULL;
#else
    pval        *pParam = NULL;
#endif

    int         nParamCount = 0;
    int         i = 0;
    PJ          *pj = NULL;
    
    char        **papszBuf = NULL;


/* -------------------------------------------------------------------- */
/*      extract parameters.                                             */
/* -------------------------------------------------------------------- */
    if (getParameters(ht, 1, &pArrayOfParams) != SUCCESS)
    {           
        WRONG_PARAM_COUNT;
    }

    if (pArrayOfParams->type == IS_ARRAY)
        nParamCount = _php3_hash_num_elements(pArrayOfParams->value.ht);
    else
      nParamCount = 0;

    if (nParamCount <= 0)
      RETURN_LONG(-1);

    papszBuf = (char **) malloc((nParamCount+2)*sizeof(char *));
	
    for (i = 0; i < nParamCount; i++) 
    {
        if (_php3_hash_index_find(pArrayOfParams->value.ht, i, 
                                  (void **)&pParam) != FAILURE)
        {
#ifdef PHP4
            convert_to_string((*pParam));
            if ((*pParam)->value.str.val != NULL)
              papszBuf[i] = strdup((*pParam)->value.str.val);
#else
            convert_to_string(pParam);
            if (pParam->value.str.val != NULL)
              papszBuf[i] = strdup(pParam->value.str.val);
#endif
        }
    }
     papszBuf[i] = NULL;

    pj = pj_init(nParamCount, papszBuf);
           
    _php_proj_build_proj_object(pj, list, return_value);  
}


/************************************************************************/
/*       DLEXPORT void php_proj_pj_fwd(INTERNAL_FUNCTION_PARAMETERS)    */
/*                                                                      */
/*       Performs a projection from lat/long coordinates to             */
/*      cartesian coordinates (projection defines in the pj parameter)  */
/*      Parameters :                                                    */
/*                                                                      */
/*         - double p1 : latitude (in decimal degree )                  */
/*         - double p2 : longitude (in decimal degree )                 */
/*         - PJ pj : valid projection structure (see pj_init)           */
/*                                                                      */
/*       Ex :                                                           */
/*              $lat = 45.25;                                           */
/*              $lon = -75.42;                                          */
/*                                                                      */
/*              $ret = pj_fwd($lat, $lon, $pj);                         */
/*              printf("geo x = %f<br>\n", $ret["u"]);                  */
/*              printf("geo y = %f<br>\n",$ret["v"]);                   */
/*                                                                      */
/************************************************************************/
DLEXPORT void php_proj_pj_fwd(INTERNAL_FUNCTION_PARAMETERS)
{
#ifdef PHP4
    HashTable   *list=NULL;
#endif
    pval        *p1, *p2;
    pval        *pj = NULL;
    PJ          *popj = NULL;
    projUV      pnt;
    projUV      pntReturn = {0,0};

/* -------------------------------------------------------------------- */
/*      extract parameters.                                             */
/* -------------------------------------------------------------------- */
    if (getParameters(ht, 3, &p1, &p2, &pj) != SUCCESS)
    {           
        WRONG_PARAM_COUNT;
    }

/* -------------------------------------------------------------------- */
/*      initilize return array.                                         */
/* -------------------------------------------------------------------- */
    if (array_init(return_value) == FAILURE) 
    {
        RETURN_FALSE;
    }
    
    convert_to_double(p1);
    convert_to_double(p2);

    popj = (PJ *)_phpms_fetch_handle(pj, 
                                     PHPMS_GLOBAL(le_projobj), list);

    if (popj)
    {
        pnt.u = p2->value.dval * DEG_TO_RAD;
        pnt.v = p1->value.dval * DEG_TO_RAD;

        pntReturn = pj_fwd(pnt, popj);
    }

    add_assoc_double(return_value, "u", pntReturn.u);
    add_assoc_double(return_value, "v", pntReturn.v);
}


/************************************************************************/
/*       DLEXPORT void php_proj_pj_inv(INTERNAL_FUNCTION_PARAMETERS)    */
/*                                                                      */
/*       Performs a projection from cartesian  coordinates              */
/*      (projection defines in the pj parameter) to lat/long            */
/*      coordinates.                                                    */
/*                                                                      */
/*       Return vales are in decimal degrees.                           */
/*                                                                      */
/*       Parameters :                                                   */
/*                                                                      */
/*               - double p1 : projected coordinates (x)                */
/*               - double p2 : projected coordinates (y)                */
/*               - PJ pj : valid projection structure (see pj_init)     */
/*                                                                      */
/*         Ex :                                                         */
/*              $ingeox = 1537490.335842;                               */
/*              $ingeoy = -181633.471555;                               */
/*                                                                      */
/*              $ret = pj_inv($ingeox, $ingeoy, $pj);                   */
/*                                                                      */
/*              printf("latitude = %f<br>\n", $ret["u"]);               */
/*              printf("longitude = %f<br>\n",$ret["v"]);               */
/************************************************************************/
DLEXPORT void php_proj_pj_inv(INTERNAL_FUNCTION_PARAMETERS)
{
#ifdef PHP4
    HashTable   *list=NULL;
#endif
    pval        *p1, *p2;
    pval        *pj = NULL;
    PJ          *popj = NULL;
    projUV      pnt;
    projUV      pntReturn = {0,0};
/* -------------------------------------------------------------------- */
/*      extract parameters.                                             */
/* -------------------------------------------------------------------- */
    if (getParameters(ht, 3, &p1, &p2, &pj) != SUCCESS)
    {           
        WRONG_PARAM_COUNT;
    }
    
/* -------------------------------------------------------------------- */
/*      initilize return array.                                         */
/* -------------------------------------------------------------------- */
    if (array_init(return_value) == FAILURE) 
    {
        RETURN_FALSE;
    }

    convert_to_double(p1);
    convert_to_double(p2);

    popj = (PJ *)_phpms_fetch_handle(pj, 
                                     PHPMS_GLOBAL(le_projobj), list);

    if (popj)
    {
        pnt.u = p1->value.dval;
        pnt.v = p2->value.dval;

        pntReturn = pj_inv(pnt, popj);
        pntReturn.u *= RAD_TO_DEG;
        pntReturn.v *= RAD_TO_DEG;
    }

    add_assoc_double(return_value, "u", pntReturn.v);
    add_assoc_double(return_value, "v", pntReturn.u);
}


/************************************************************************/
/*       DLEXPORT void php_proj_pj_free(INTERNAL_FUNCTION_PARAMETERS)   */
/************************************************************************/
DLEXPORT void php_proj_pj_free(INTERNAL_FUNCTION_PARAMETERS)
{
#ifdef PHP4
    HashTable   *list=NULL;
#endif

    pval        *pj = NULL;
    PJ          *popj = NULL;
/* -------------------------------------------------------------------- */
/*      extract parameters.                                             */
/* -------------------------------------------------------------------- */
    if (getParameters(ht, 1, &pj) != SUCCESS)
    {           
        WRONG_PARAM_COUNT;
    }
    
    popj = (PJ *)_phpms_fetch_handle(pj, 
                                     PHPMS_GLOBAL(le_projobj), list);

    if (popj)
    {
        pj_free(popj);
    }

}

#endif /* USE_PROJ */




/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  PHP/MapScript extension for MapServer : Utility functions
 * Author:   Daniel Morissette, DM Solutions Group (dmorissette@dmsolutions.ca)
 *
 **********************************************************************
 * Copyright (c) 2000-2005, DM Solutions Group
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/


#include "php_mapscript_util.h"
#include "maperror.h"


/*=====================================================================
 *                       Misc support functions
 *====================================================================*/

/**********************************************************************
 *                     _phpms_report_mapserver_error()
 **********************************************************************/
void _phpms_report_mapserver_error(int php_err_type)
{
    errorObj *ms_error;

    ms_error = msGetErrorObj();

    while (ms_error && ms_error->code != MS_NOERR)
    {
        php3_error(php_err_type, 
                   "[MapServer Error]: %s: %s\n", 
                   ms_error->routine, ms_error->message);
        ms_error = ms_error->next;
    }
}




/**********************************************************************
 *                     _phpms_fetch_handle2()
 **********************************************************************/ 
void *_phpms_fetch_handle2(pval *pObj, 
                           int handle_type1, int handle_type2,
                            HashTable *list TSRMLS_DC)
                                  
{
    pval **phandle;
    void *retVal = NULL;

    if (pObj->type != IS_OBJECT)
    {
        php3_error(E_ERROR, "Object expected as argument.");
        retVal = NULL;
    }
    else if (zend_hash_find(Z_OBJPROP_P(pObj), "_handle_", 
                            sizeof("_handle_"), 
                            (void *)&phandle) == FAILURE)
    {
        php3_error(E_ERROR, 
                   "Unable to find _handle_ property");
        retVal = NULL;
    }
    else
    {
        int type;
        retVal = (void *)php3_list_find((*phandle)->value.lval, &type);
                
        if (retVal == NULL || (type != handle_type1 && type != handle_type2))
        {
            php3_error(E_ERROR, "Object has an invalid _handle_ property");
            retVal = NULL;
        }
    }

    /* Note: because of the php3_error() calls above, this function
     *       will probably never return a NULL value.
     */
    return retVal;    
}

/**********************************************************************
 *                     _phpms_fetch_handle()
 **********************************************************************/
void *_phpms_fetch_handle(pval *pObj, int handle_type, 
                          HashTable *list TSRMLS_DC)
{
    return _phpms_fetch_handle2(pObj, handle_type, handle_type, list TSRMLS_CC);
}


/**********************************************************************
 *                     _phpms_fetch_property_handle2()
 **********************************************************************/
char *_phpms_fetch_property_handle2(pval *pObj, char *property_name, 
                                    int handle_type1, int handle_type2,
                                    HashTable *list TSRMLS_DC, int err_type)
{
    pval **phandle;
    void *retVal = NULL;
    int type;

    if (pObj->type != IS_OBJECT)
    {
        php3_error(err_type, "Object expected as argument.");
        return NULL;
    }
    else if (zend_hash_find(Z_OBJPROP_P(pObj), property_name, 
                            strlen(property_name)+1, 
                            (void *)&phandle) == FAILURE)
    {
        if (err_type != 0)
            php3_error(err_type, "Unable to find %s property", property_name);
        return NULL;
    }

    if ((*phandle)->type != IS_RESOURCE ||
        (retVal = (void *)php3_list_find((*phandle)->value.lval, &type)) ==NULL ||
        (type != handle_type1 && type != handle_type2))
    {
        if (err_type != 0)
            php3_error(err_type, "Object has an invalid '%s' property", 
                       property_name);
        retVal = NULL;
    }

    return retVal;
}

/**********************************************************************
 *                     _phpms_fetch_property_handle()
 **********************************************************************/
char *_phpms_fetch_property_handle(pval *pObj, char *property_name, 
                                   int handle_type, HashTable *list
                                   TSRMLS_DC, int err_type)
{
    return _phpms_fetch_property_handle2(pObj, property_name, 
                                         handle_type, handle_type, list
                                         TSRMLS_CC, err_type);
}

/**********************************************************************
 *                     _phpms_fetch_property_string()
 **********************************************************************/
char *_phpms_fetch_property_string(pval *pObj, char *property_name, 
                                   int err_type TSRMLS_DC)
{

    pval **phandle;

    if (pObj->type != IS_OBJECT)
    {
        php3_error(err_type, "Object expected as argument.");
        return "";
    }
    else if (zend_hash_find(Z_OBJPROP_P(pObj), 
                            property_name, strlen(property_name)+1, 
                            (void *)&phandle) == FAILURE)
    {
        if (err_type != 0)
            php3_error(err_type, "Unable to find %s property", property_name);
        return "";
    }

    convert_to_string((*phandle));
    return (*phandle)->value.str.val;
}

/**********************************************************************
 *                     _phpms_fetch_property_long()
 **********************************************************************/
long _phpms_fetch_property_long(pval *pObj, char *property_name, 
                                int err_type TSRMLS_DC)
{
    pval **phandle;

    if (pObj->type != IS_OBJECT)
    {
        php3_error(err_type, "Object expected as argument.");
        return 0;
    }
    else if (zend_hash_find(Z_OBJPROP_P(pObj), property_name, 
                            strlen(property_name)+1, 
                            (void *)&phandle) == FAILURE)
    {
        if (err_type != 0)
            php3_error(err_type, "Unable to find %s property", property_name);
        return 0;
    }

    if ((*phandle)->type == IS_RESOURCE)
    {
        php3_error(err_type, 
                   "ERROR: Property %s is of type IS_RESOURCE.  "
                   "It cannot be handled as LONG", property_name);
        return 0;
    }
    convert_to_long(*phandle);
    return (*phandle)->value.lval;
}

/**********************************************************************
 *                     _phpms_fetch_property_double()
 **********************************************************************/
double _phpms_fetch_property_double(pval *pObj, char *property_name,
                                    int err_type TSRMLS_DC)
{
    pval **phandle;

    if (pObj->type != IS_OBJECT)
    {
        php3_error(err_type, "Object expected as argument.");
        return 0.0;
    }
    else if (zend_hash_find(Z_OBJPROP_P(pObj), property_name, 
                            strlen(property_name)+1, 
                            (void *)&phandle) == FAILURE)
   {
        if (err_type != 0)
            php3_error(err_type, "Unable to find %s property", property_name);
        return 0.0;
    }

    convert_to_double(*phandle);
    return (*phandle)->value.dval;
}

/**********************************************************************
 *                     _phpms_fetch_property_resource()
 **********************************************************************/
long _phpms_fetch_property_resource(pval *pObj, char *property_name, 
                                    int err_type TSRMLS_DC)
{
    pval **phandle;

    if (pObj->type != IS_OBJECT)
    {
        php3_error(err_type, "Object expected as argument.");
        return 0;
    }
    else if (zend_hash_find(Z_OBJPROP_P(pObj), property_name, 
                            strlen(property_name)+1, 
                            (void *)&phandle) == FAILURE)
    {
        if (err_type != 0)
            php3_error(err_type, "Unable to find %s property", property_name);
        return 0;
    }

    if ((*phandle)->type != IS_RESOURCE)
    {
        if (err_type != 0)
            php3_error(err_type, 
                       "Property %s has invalid type.  Expected IS_RESOURCE.",
                       property_name);
        return 0;
    }
    return (*phandle)->value.lval;
}




/**********************************************************************
 *                     _phpms_set_property_string()
 **********************************************************************/
int _phpms_set_property_string(pval *pObj, char *property_name, 
                               char *szNewValue, int err_type TSRMLS_DC)
{
    pval **phandle;

    if (pObj->type != IS_OBJECT)
    {
        php3_error(err_type, "Object expected as argument.");
        return -1;
    }
    else if (zend_hash_find(Z_OBJPROP_P(pObj), property_name, 
                            strlen(property_name)+1, 
                            (void *)&phandle) == FAILURE)
    {
        if (err_type != 0)
            php3_error(err_type, "Unable to find %s property", property_name);
        return -1;
    }

    SEPARATE_ZVAL(phandle);
    zval_dtor(*phandle);
    ZVAL_STRING(*phandle, szNewValue, 1);

    return 0;
}

/**********************************************************************
 *                     _phpms_set_property_null()
 **********************************************************************/
int _phpms_set_property_null(pval *pObj, char *property_name, int err_type TSRMLS_DC)
{
    pval **phandle;

    if (pObj->type != IS_OBJECT)
    {
        php3_error(err_type, "Object expected as argument.");
        return -1;
    }
    else if (zend_hash_find(Z_OBJPROP_P(pObj), property_name, 
                            strlen(property_name)+1, 
                            (void *)&phandle) == FAILURE)
    {
        if (err_type != 0)
            php3_error(err_type, "Unable to find %s property", property_name);
        return -1;
    }

    SEPARATE_ZVAL(phandle);
    zval_dtor(*phandle);
    ZVAL_NULL(*phandle);

    return 0;
}

/**********************************************************************
 *                     _phpms_set_property_long()
 **********************************************************************/
int _phpms_set_property_long(pval *pObj, char *property_name, 
                             long lNewValue, int err_type TSRMLS_DC)
{
    pval **phandle;

    if (pObj->type != IS_OBJECT)
    {
        php3_error(err_type, "Object expected as argument.");
        return -1;
    }
    else if (zend_hash_find(Z_OBJPROP_P(pObj), property_name, 
                            strlen(property_name)+1, 
                            (void *)&phandle) == FAILURE)
    {
        if (err_type != 0)
            php3_error(err_type, "Unable to find %s property", property_name);
        return -1;
    }

    SEPARATE_ZVAL(phandle);
    zval_dtor(*phandle);
    ZVAL_LONG(*phandle, lNewValue);

    return 0;
}

/**********************************************************************
 *                     _phpms_set_property_double()
 **********************************************************************/
int _phpms_set_property_double(pval *pObj, char *property_name, 
                               double dNewValue, int err_type TSRMLS_DC)
{
    pval **phandle;

    if (pObj->type != IS_OBJECT)
    {
        php3_error(err_type, "Object expected as argument.");
        return -1;
    }
    else if (zend_hash_find(Z_OBJPROP_P(pObj), property_name, 
                            strlen(property_name)+1, 
                            (void *)&phandle) == FAILURE)
    {
        if (err_type != 0)
            php3_error(err_type, "Unable to find %s property", property_name);
        return -1;
    }

    SEPARATE_ZVAL(phandle);
    zval_dtor(*phandle);
    ZVAL_DOUBLE(*phandle, dNewValue);

    return 0;
}

/**********************************************************************
 *                     _phpms_add_property_object()
 **********************************************************************/
int _phpms_add_property_object(pval *pObj,      
                               char *property_name, pval *pObjToAdd,
                               int err_type TSRMLS_DC)
{
    if (add_property_zval(pObj, property_name, pObjToAdd) == FAILURE)
    {
        if (err_type != 0)
          php3_error(err_type, "Unable to add %s property", property_name);
        return -1;
    }
#if PHP_MAJOR_VERSION >= 5
    else
        ZVAL_DELREF(pObjToAdd);
#endif
    return 0;
}


/**********************************************************************
 *                     _phpms_object_init()
 *
 * Cover function for PHP3 and PHP4's object_init() functions.
 *
 * PHP4: Make sure object has been allocated using MAKE_STD_ZVAL(), note
 * that return_value passed by PHP is already pre-initialized.
 **********************************************************************/
int _phpms_object_init(pval *return_value, int  handle_id,
                       function_entry *class_functions,
                       void *zend_class_entry_ptr TSRMLS_DC)
{
    zend_class_entry *new_class_entry_ptr;
    new_class_entry_ptr = (zend_class_entry *)zend_class_entry_ptr;

    object_init_ex(return_value, new_class_entry_ptr);
    add_property_resource(return_value, "_handle_", handle_id);

    return 0;
}

 

/************************************************************************/
/*          int _php_extract_associate_array(HashTable *php, char       */
/*      **array)                                                        */
/*                                                                      */
/*      Translate a PHP associate array (HashTable *) into an array.    */
/*      Ex if php array :  tttarray["first"] = "1st";                   */
/*                         tttarray["second"] = "2nd";                  */
/*                                                                      */
/*        the resultiong array will be :                                */
/*          array[0] = "first";                                         */
/*          array[1] = "1st"                                            */
/*          array[2] = "second";                                        */
/*          array[3] = "2nd"                                            */
/*                                                                      */
/************************************************************************/
int _php_extract_associative_array(HashTable *php, char **array)
{
/* -------------------------------------------------------------------- */
/*      Note : code extacted from sablot.c (functions related to        */
/*      xslt)                                                           */
/* -------------------------------------------------------------------- */
    zval **value;
    char *string_key = NULL;
    ulong num_key;
    int i = 0;
    
    for (zend_hash_internal_pointer_reset(php);
         zend_hash_get_current_data(php, (void *)&value) == SUCCESS;
         zend_hash_move_forward(php)) 
    {
        SEPARATE_ZVAL(value);
        convert_to_string_ex(value);
        
        switch (zend_hash_get_current_key(php, &string_key, &num_key, 1)) 
        {
            case HASH_KEY_IS_STRING:
                array[i++] = string_key;
                array[i++] = Z_STRVAL_PP(value);
                break;
        }
    }
    array[i++] = NULL;

    return 1;
}


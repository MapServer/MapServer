/**********************************************************************
 * $Id$
 *
 * Name:     php_mapscript_util.c
 * Project:  PHP/MapScript extension for MapServer : Utility functions
 * Language: ANSI C
 * Purpose:  Utility functions
 * Author:   Daniel Morissette, morissette@dmsolutions.ca
 *
 **********************************************************************
 * Copyright (c) 2000-2002, DM Solutions Group
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
 * Revision 1.14  2002/10/28 20:31:22  dan
 * New support for WMS Map Context (from Julien)
 *
 * Revision 1.3  2002/10/22 20:03:57  julien
 * Add the mapcontext support
 *
 * Revision 1.13  2002/03/08 23:16:41  assefa
 * Add PHP4.1 support.
 *
 * Revision 1.12  2002/03/07 22:31:01  assefa
 * Add template processing functions.
 *
 * Revision 1.11  2002/01/22 21:19:02  sacha
 * Add two functions in maplegend.c
 * - msDrawLegendIcon that draw an class legend icon over an existing image.
 * - msCreateLegendIcon that draw an class legend icon and return the newly
 * created image.
 * Also, an php examples in mapscript/php3/examples/test_draw_legend_icon.phtml
 *
 * Revision 1.10  2002/01/11 17:30:47  dan
 * Replace global ms_error with thread-safe msGetErrorObj() call
 *
 * Revision 1.9  2001/09/13 20:56:04  dan
 * Fixed _phpms_add_property_object() for PHP4 (thanks to Zeev Suraski) and
 * added _phpms_fetch_property_resource().  (See bug#30 and #40)
 *
 * Revision 1.8  2001/09/05 19:59:12  dan
 * Another manifestation of the invalid _map_handle_ problem: Check for type
 * IS_RESOURCE instead of IS_LONG in _phpms_fetch_property_handle2().
 *
 * Revision 1.7  2001/08/21 19:07:18  dan
 * Reset ms_error struct at the end of _phpms_report_mapserver_error()
 *
 * Revision 1.6  2001/07/20 13:50:27  dan
 * Call zend_list_addref() when creating resource member variables
 *
 * Revision 1.5  2001/03/18 17:48:46  dan
 * Fixed crash with PHP4 version of _phpms_fetch_property_handle2()
 *
 * Revision 1.4  2001/03/13 16:52:15  dan
 * Added ZVAL_ADDREF() in add_property_object()
 *
 * Revision 1.3  2001/03/12 19:02:46  dan
 * Added query-related stuff in PHP MapScript
 *
 * Revision 1.2  2000/09/08 21:27:54  dan
 * Added _phpms_object_init()
 *
 * Revision 1.1  2000/09/06 19:44:07  dan
 * Ported module to PHP4
 *
 *
 */

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

    if (ms_error->code != MS_NOERR)
    {
        php3_error(php_err_type, 
                   "MapServer Error in %s: %s\n", 
                   ms_error->routine, ms_error->message);
        ms_error->code = -1;
        strcpy(ms_error->message, "");
        strcpy(ms_error->routine, "");
    }
}




/**********************************************************************
 *                     _phpms_fetch_handle2()
 **********************************************************************/ 
#ifdef PHP4
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
    else if (zend_hash_find(pObj->value.obj.properties, "_handle_", 
                            sizeof("_handle_"), 
                            (void **)&phandle) == FAILURE)
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
#else

void *_phpms_fetch_handle2(pval *pObj, 
                           int handle_type1, int handle_type2, 
                           HashTable *list)
{
    pval *phandle;
    void *retVal = NULL;

    if (pObj->type != IS_OBJECT)
    {
        php3_error(E_ERROR, "Object expected as argument.");
        retVal = NULL;
    }
    else if (_php3_hash_find(pObj->value.ht, "_handle_", sizeof("_handle_"), 
                             (void **)&phandle) == FAILURE)
    {
        php3_error(E_ERROR, 
                   "Unable to find _handle_ property");
        retVal = NULL;
    }
    else
    {
        int type;
        retVal = (void *)php3_list_find(phandle->value.lval, &type);
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
#endif

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
#ifdef PHP4
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
    else if (zend_hash_find(pObj->value.obj.properties, property_name, 
                            strlen(property_name)+1, 
                            (void **)&phandle) == FAILURE)
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
#else
char *_phpms_fetch_property_handle2(pval *pObj, char *property_name,
                                    int handle_type1, int handle_type2,
                                    HashTable *list, int err_type)
{
    pval *phandle;
    void *retVal = NULL;
    int type;

    if (pObj->type != IS_OBJECT)
    {
        php3_error(err_type, "Object expected as argument.");
        return NULL;
    }
    else if (_php3_hash_find(pObj->value.ht, property_name, 
                             strlen(property_name)+1, 
                             (void **)&phandle) == FAILURE)
    {
        if (err_type != 0)
            php3_error(err_type, "Unable to find %s property", property_name);
        return NULL;
    }

    if (phandle->type != IS_LONG ||
        (retVal = (void *)php3_list_find(phandle->value.lval, &type)) ==NULL ||
        (type != handle_type1 && type != handle_type2))
    {
        if (err_type != 0)
            php3_error(err_type, "Object has an invalid '%s' property", 
                       property_name);
        retVal = NULL;
    }

    return retVal;
}
#endif

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
#ifdef PHP4
char *_phpms_fetch_property_string(pval *pObj, char *property_name, 
                                   int err_type)
{
    pval **phandle;

    if (pObj->type != IS_OBJECT)
    {
        php3_error(err_type, "Object expected as argument.");
        return "";
    }
    else if (zend_hash_find(pObj->value.obj.properties, 
                            property_name, strlen(property_name)+1, 
                            (void **)&phandle) == FAILURE)
    {
        if (err_type != 0)
            php3_error(err_type, "Unable to find %s property", property_name);
        return "";
    }

    convert_to_string((*phandle));
    return (*phandle)->value.str.val;
}
#else
char *_phpms_fetch_property_string(pval *pObj, char *property_name, 
                                   int err_type)
{
    pval *phandle;

    if (pObj->type != IS_OBJECT)
    {
        php3_error(err_type, "Object expected as argument.");
        return "";
    }
    else if (_php3_hash_find(pObj->value.ht, property_name, 
                             strlen(property_name)+1, 
                             (void **)&phandle) == FAILURE)
    {
        if (err_type != 0)
            php3_error(err_type, "Unable to find %s property", property_name);
        return "";
    }

    convert_to_string(phandle);
    return phandle->value.str.val;
}
#endif

/**********************************************************************
 *                     _phpms_fetch_property_long()
 **********************************************************************/
long _phpms_fetch_property_long(pval *pObj, char *property_name, 
                                int err_type)
{
#ifdef PHP4    
    pval **phandle;
#else
    pval *phandle;
#endif

    if (pObj->type != IS_OBJECT)
    {
        php3_error(err_type, "Object expected as argument.");
        return 0;
    }
#ifdef PHP4
    else if (zend_hash_find(pObj->value.obj.properties, property_name, 
                            strlen(property_name)+1, 
                            (void **)&phandle) == FAILURE)
#else
    else if (_php3_hash_find(pObj->value.ht, property_name, 
                             strlen(property_name)+1, 
                             (void **)&phandle) == FAILURE)
#endif
    {
        if (err_type != 0)
            php3_error(err_type, "Unable to find %s property", property_name);
        return 0;
    }

#ifdef PHP4
    if ((*phandle)->type == IS_RESOURCE)
    {
        php3_error(err_type, 
                   "ERROR: Property %s is of type IS_RESOURCE.  "
                   "It cannot be handled as LONG", property_name);
        return 0;
    }
    convert_to_long(*phandle);
    return (*phandle)->value.lval;
#else
    convert_to_long(phandle);
    return phandle->value.lval;
#endif
}

/**********************************************************************
 *                     _phpms_fetch_property_double()
 **********************************************************************/
double _phpms_fetch_property_double(pval *pObj, char *property_name,
                                    int err_type)
{
#ifdef PHP4    
    pval **phandle;
#else
    pval *phandle;
#endif

    if (pObj->type != IS_OBJECT)
    {
        php3_error(err_type, "Object expected as argument.");
        return 0.0;
    }
#ifdef PHP4
    else if (zend_hash_find(pObj->value.obj.properties, property_name, 
                            strlen(property_name)+1, 
                            (void **)&phandle) == FAILURE)
#else
    else if (_php3_hash_find(pObj->value.ht, property_name, 
                             strlen(property_name)+1, 
                             (void **)&phandle) == FAILURE)
#endif
   {
        if (err_type != 0)
            php3_error(err_type, "Unable to find %s property", property_name);
        return 0.0;
    }

#ifdef PHP4
    convert_to_double(*phandle);
    return (*phandle)->value.dval;
#else
    convert_to_double(phandle);
    return phandle->value.dval;
#endif
}

/**********************************************************************
 *                     _phpms_fetch_property_resource()
 **********************************************************************/
long _phpms_fetch_property_resource(pval *pObj, char *property_name, 
                                    int err_type)
{
#ifdef PHP4    
    pval **phandle;
#else
    pval *phandle;
#endif

    if (pObj->type != IS_OBJECT)
    {
        php3_error(err_type, "Object expected as argument.");
        return 0;
    }
#ifdef PHP4
    else if (zend_hash_find(pObj->value.obj.properties, property_name, 
                            strlen(property_name)+1, 
                            (void **)&phandle) == FAILURE)
#else
    else if (_php3_hash_find(pObj->value.ht, property_name, 
                             strlen(property_name)+1, 
                             (void **)&phandle) == FAILURE)
#endif
    {
        if (err_type != 0)
            php3_error(err_type, "Unable to find %s property", property_name);
        return 0;
    }

#ifdef PHP4
    if ((*phandle)->type != IS_RESOURCE)
    {
        if (err_type != 0)
            php3_error(err_type, 
                       "Property %s has invalid type.  Expected IS_RESOURCE.",
                       property_name);
        return 0;
    }
    return (*phandle)->value.lval;
#else
    convert_to_long(phandle);
    return phandle->value.lval;
#endif
}




/**********************************************************************
 *                     _phpms_set_property_string()
 **********************************************************************/
int _phpms_set_property_string(pval *pObj, char *property_name, 
                               char *szNewValue, int err_type)
{
#ifdef PHP4    
    pval **phandle;
#else
    pval *phandle;
#endif

    if (pObj->type != IS_OBJECT)
    {
        php3_error(err_type, "Object expected as argument.");
        return -1;
    }
#ifdef PHP4
    else if (zend_hash_find(pObj->value.obj.properties, property_name, 
                            strlen(property_name)+1, 
                            (void **)&phandle) == FAILURE)
#else
    else if (_php3_hash_find(pObj->value.ht, property_name, 
                             strlen(property_name)+1, 
                             (void **)&phandle) == FAILURE)
#endif
    {
        if (err_type != 0)
            php3_error(err_type, "Unable to find %s property", property_name);
        return -1;
    }

#ifdef PHP4
    convert_to_string(*phandle);
    if ((*phandle)->value.str.val)
        efree((*phandle)->value.str.val);
    (*phandle)->value.str.val = estrdup(szNewValue);
    (*phandle)->value.str.len = strlen(szNewValue);
#else
    convert_to_string(phandle);
    if (phandle->value.str.val)
        efree(phandle->value.str.val);
    phandle->value.str.val = estrdup(szNewValue);
    phandle->value.str.len = strlen(szNewValue);
#endif
    return 0;
}

/**********************************************************************
 *                     _phpms_set_property_long()
 **********************************************************************/
int _phpms_set_property_long(pval *pObj, char *property_name, 
                             long lNewValue, int err_type)
{
#ifdef PHP4    
    pval **phandle;
#else
    pval *phandle;
#endif


    if (pObj->type != IS_OBJECT)
    {
        php3_error(err_type, "Object expected as argument.");
        return -1;
    }
#ifdef PHP4
    else if (zend_hash_find(pObj->value.obj.properties, property_name, 
                            strlen(property_name)+1, 
                            (void **)&phandle) == FAILURE)
#else
    else if (_php3_hash_find(pObj->value.ht, property_name, 
                             strlen(property_name)+1, 
                             (void **)&phandle) == FAILURE)
#endif
    {
        if (err_type != 0)
            php3_error(err_type, "Unable to find %s property", property_name);
        return -1;
    }

#ifdef PHP4
    convert_to_long(*phandle);
    (*phandle)->value.lval = lNewValue;
#else
    convert_to_long(phandle);
    phandle->value.lval = lNewValue;
#endif

    return 0;
}

/**********************************************************************
 *                     _phpms_set_property_double()
 **********************************************************************/
int _phpms_set_property_double(pval *pObj, char *property_name, 
                               double dNewValue, int err_type)
{
#ifdef PHP4    
    pval **phandle;
#else
    pval *phandle;
#endif

    if (pObj->type != IS_OBJECT)
    {
        php3_error(err_type, "Object expected as argument.");
        return -1;
    }
#ifdef PHP4
    else if (zend_hash_find(pObj->value.obj.properties, property_name, 
                            strlen(property_name)+1, 
                            (void **)&phandle) == FAILURE)
#else
    else if (_php3_hash_find(pObj->value.ht, property_name, 
                             strlen(property_name)+1, 
                             (void **)&phandle) == FAILURE)
#endif
    {
        if (err_type != 0)
            php3_error(err_type, "Unable to find %s property", property_name);
        return -1;
    }

#ifdef PHP4
     convert_to_double(*phandle);
     (*phandle)->value.dval = dNewValue;
#else
    convert_to_double(phandle);
    phandle->value.dval = dNewValue;
#endif

    return 0;
}

/**********************************************************************
 *                     _phpms_add_property_object()
 **********************************************************************/
#ifdef PHP4
int _phpms_add_property_object(pval *pObj,      
                               char *property_name, pval *pObjToAdd,
                               int err_type)
{
    if (add_property_zval(pObj, property_name, pObjToAdd) == FAILURE)
    {
        if (err_type != 0)
          php3_error(err_type, "Unable to add %s property", property_name);
        return -1;
    }

    return 0;
}
#else
int _phpms_add_property_object(pval *pObj,      
                               char *property_name, pval *pObjToAdd,
                               int err_type)
{
    pval *phandle;

    /* This is kind of a hack...
     * We will add a 'long' property, and then we'll replace its contents 
     * with the object that was passed.
     */

    if (pObj->type != IS_OBJECT || 
        (pObjToAdd->type != IS_OBJECT && pObjToAdd->type != IS_ARRAY))
    {
        php3_error(err_type, "Object or array expected as argument.");
        return -1;
    }
    else if (add_property_long(pObj, property_name, 0) == FAILURE ||
             _php3_hash_find(pObj->value.ht, property_name, 
                             strlen(property_name)+1, 
                             (void **)&phandle) == FAILURE)
    {
        if (err_type != 0)
            php3_error(err_type, "Unable to add %s property", property_name);
        return -1;
    }

    phandle->type = pObjToAdd->type;
    phandle->value.ht = pObjToAdd->value.ht;

    return 0;
}
#endif


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
                       void *zend_class_entry_ptr)
{
#ifdef PHP4
    zend_class_entry *new_class_entry_ptr;
    new_class_entry_ptr = (zend_class_entry *)zend_class_entry_ptr;

    object_init_ex(return_value, new_class_entry_ptr);
    add_property_resource(return_value, "_handle_", handle_id);
#else
    object_init(return_value);
    add_property_long(return_value, "_handle_", handle_id);

    while(class_functions && class_functions->fname != NULL)
    {
        add_method(return_value, 
                   class_functions->fname, class_functions->handler );

        class_functions++;
    }
#endif

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
#ifdef PHP4
    zval **value;
    char *string_key = NULL;
    ulong num_key;
    int i = 0;
    
    for (zend_hash_internal_pointer_reset(php);
         zend_hash_get_current_data(php, (void **)&value) == SUCCESS;
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

#endif
    return 1;
}






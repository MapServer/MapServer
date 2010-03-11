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


zend_object_value mapscript_object_new(zend_object *zobj,
                                       zend_class_entry *ce,
                                       void (*zend_objects_free_object))
{
    zend_object_value retval;
    zval *temp;
    
    zobj->ce = ce;
    ALLOC_HASHTABLE(zobj->properties);
    zend_hash_init(zobj->properties, 0, NULL, ZVAL_PTR_DTOR, 0);
    zend_hash_copy(zobj->properties, &ce->default_properties, (copy_ctor_func_t) zval_add_ref,(void *) &temp, sizeof(zval *));
    retval.handle = zend_objects_store_put(zobj, NULL, (zend_objects_free_object_storage_t)zend_objects_free_object, NULL TSRMLS_CC);
    retval.handlers = &mapscript_std_object_handlers;
    return retval;
}

int mapscript_extract_associative_array(HashTable *php, char **array)
{
    zval **value;
    char *string_key = NULL;
    ulong num_key;
    int i = 0;    

    for(zend_hash_internal_pointer_reset(php); 
        zend_hash_has_more_elements(php) == SUCCESS; 
        zend_hash_move_forward(php))
    { 
        zend_hash_get_current_data(php, (void **)&value);

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

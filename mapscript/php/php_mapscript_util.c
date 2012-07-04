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
                                       void (*zend_objects_free_object) TSRMLS_DC)
{
    zend_object_value retval;
    zval *temp;
    
    zobj->ce = ce;
    ALLOC_HASHTABLE(zobj->properties);
    zend_hash_init(zobj->properties, 0, NULL, ZVAL_PTR_DTOR, 0);
    //handle changes in PHP 5.4.x
    #if PHP_VERSION_ID < 50399
      zend_hash_copy(zobj->properties, &ce->default_properties, (copy_ctor_func_t) zval_add_ref,(void *) &temp, sizeof(zval *));
    #else
      object_properties_init(zobj, ce);
    #endif
    retval.handle = zend_objects_store_put(zobj, NULL, (zend_objects_free_object_storage_t)zend_objects_free_object, NULL TSRMLS_CC);
    retval.handlers = &mapscript_std_object_handlers;
    return retval;
}

zend_object_value mapscript_object_new_ex(zend_object *zobj,
                                          zend_class_entry *ce,
                                          void (*zend_objects_free_object),
                                          zend_object_handlers *object_handlers TSRMLS_DC)
{
    zend_object_value retval;
    zval *temp;
    
    zobj->ce = ce;
    ALLOC_HASHTABLE(zobj->properties);
    zend_hash_init(zobj->properties, 0, NULL, ZVAL_PTR_DTOR, 0);
    //handle changes in PHP 5.4.x
    #if PHP_VERSION_ID < 50399
      zend_hash_copy(zobj->properties, &ce->default_properties, (copy_ctor_func_t) zval_add_ref,(void *) &temp, sizeof(zval *));
    #else
      object_properties_init(zobj, ce);
    #endif
    retval.handle = zend_objects_store_put(zobj, NULL, (zend_objects_free_object_storage_t)zend_objects_free_object, NULL TSRMLS_CC);
    retval.handlers = object_handlers;
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

/* This method returns an object property of a php class. If the object exists, it returns a reference to it, 
   otherwise it creates it */
void mapscript_fetch_object(zend_class_entry *ce, zval* zval_parent, php_layer_object* layer, 
                            void *internal_object, 
                            zval **php_object_storage TSRMLS_DC)
{
    parent_object p;

    // create the parent struct
    p.val = zval_parent;
    p.child_ptr = &*php_object_storage;
    MAKE_STD_ZVAL(*php_object_storage);
        
    if (ce == mapscript_ce_outputformat)
        mapscript_create_outputformat((outputFormatObj*)internal_object, p, *php_object_storage TSRMLS_CC);
    else if (ce == mapscript_ce_color)
        mapscript_create_color((colorObj*)internal_object, p, *php_object_storage TSRMLS_CC);
    else if (ce == mapscript_ce_rect)
        mapscript_create_rect((rectObj*)internal_object, p, *php_object_storage TSRMLS_CC);
    else if (ce == mapscript_ce_class)
        mapscript_create_class((classObj*)internal_object, p, *php_object_storage TSRMLS_CC);
    else if (ce == mapscript_ce_hashtable)
        mapscript_create_hashtable((hashTableObj*)internal_object, p, *php_object_storage TSRMLS_CC);
    else if (ce == mapscript_ce_label)
        mapscript_create_label((labelObj*)internal_object, p, *php_object_storage TSRMLS_CC);
    else if (ce == mapscript_ce_style)
        mapscript_create_style((styleObj*)internal_object, p, *php_object_storage TSRMLS_CC);
    else if (ce == mapscript_ce_symbol)
        mapscript_create_symbol((symbolObj*)internal_object, p, *php_object_storage TSRMLS_CC);
    else if (ce == mapscript_ce_labelcachemember)
        mapscript_create_labelcachemember((labelCacheMemberObj*)internal_object, p, *php_object_storage TSRMLS_CC);
    else if (ce == mapscript_ce_labelcache)
        mapscript_create_labelcache((labelCacheObj*)internal_object, p, *php_object_storage TSRMLS_CC);
    else if (ce == mapscript_ce_result)
        mapscript_create_result((resultObj*)internal_object, p, *php_object_storage TSRMLS_CC);
    else if (ce == mapscript_ce_scalebar)
        mapscript_create_scalebar((scalebarObj*)internal_object, p, *php_object_storage TSRMLS_CC);
    else if (ce == mapscript_ce_web)
        mapscript_create_web((webObj*)internal_object, p, *php_object_storage TSRMLS_CC);
    else if (ce == mapscript_ce_legend)
        mapscript_create_legend((legendObj*)internal_object, p, *php_object_storage TSRMLS_CC);
    else if (ce == mapscript_ce_querymap)
        mapscript_create_querymap((queryMapObj*)internal_object, p, *php_object_storage TSRMLS_CC);
    else if (ce == mapscript_ce_grid)
        mapscript_create_grid((graticuleObj*)internal_object, p, *php_object_storage TSRMLS_CC);
    else if (ce == mapscript_ce_referencemap)
        mapscript_create_referencemap((referenceMapObj*)internal_object, p, *php_object_storage TSRMLS_CC);
    else if (ce == mapscript_ce_point)
        mapscript_create_point((pointObj*)internal_object, p, *php_object_storage TSRMLS_CC);
    else if (ce == mapscript_ce_projection)
        mapscript_create_projection((projectionObj*)internal_object, p, *php_object_storage TSRMLS_CC);
    else if (ce == mapscript_ce_line)
        mapscript_create_line((lineObj*)internal_object, p, *php_object_storage TSRMLS_CC);
    else if (ce == mapscript_ce_shape)
        mapscript_create_shape((shapeObj*)internal_object, p, layer, *php_object_storage TSRMLS_CC);
    else if (ce == mapscript_ce_layer)
        mapscript_create_layer((layerObj*)internal_object, p, *php_object_storage TSRMLS_CC);
    else if (ce == mapscript_ce_cluster)
        mapscript_create_cluster((clusterObj*)internal_object, p, *php_object_storage TSRMLS_CC);
}


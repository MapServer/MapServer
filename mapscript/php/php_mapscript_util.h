/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  PHP/MapScript extension for MapServer : Utility functions
 *           Header - macros
 * Author:   Daniel Morissette, DM Solutions Group (dmorissette@dmsolutions.ca)
 *
 **********************************************************************
 * Copyright (c) 2000, 2001, Daniel Morissette, DM Solutions Group Inc.
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


#ifndef PHP_MAPSCRIPT_UTIL_H
#define PHP_MAPSCRIPT_UTIL_H

#include "php.h"
#include "php_globals.h"
#include "php_mapscript.h"

#if ZEND_MODULE_API_NO < 20010901
#define TSRMLS_D  void
#define TSRMLS_DC
#define TSRMLS_C
#define TSRMLS_CC
#endif

/* Add pseudo refcount macros for PHP version < 5.3 */
#ifndef Z_REFCOUNT_PP

#define Z_REFCOUNT_PP(ppz)        Z_REFCOUNT_P(*(ppz))
#define Z_SET_REFCOUNT_PP(ppz, rc)    Z_SET_REFCOUNT_P(*(ppz), rc)
#define Z_ADDREF_PP(ppz)        Z_ADDREF_P(*(ppz))
#define Z_DELREF_PP(ppz)        Z_DELREF_P(*(ppz))
#define Z_ISREF_PP(ppz)         Z_ISREF_P(*(ppz))
#define Z_SET_ISREF_PP(ppz)       Z_SET_ISREF_P(*(ppz))
#define Z_UNSET_ISREF_PP(ppz)     Z_UNSET_ISREF_P(*(ppz))
#define Z_SET_ISREF_TO_PP(ppz, isref) Z_SET_ISREF_TO_P(*(ppz), isref)

#define Z_REFCOUNT_P(pz)        zval_refcount_p(pz)
#define Z_SET_REFCOUNT_P(pz, rc)    zval_set_refcount_p(pz, rc)
#define Z_ADDREF_P(pz)          zval_addref_p(pz)
#define Z_DELREF_P(pz)          zval_delref_p(pz)
#define Z_ISREF_P(pz)         zval_isref_p(pz)
#define Z_SET_ISREF_P(pz)       zval_set_isref_p(pz)
#define Z_UNSET_ISREF_P(pz)       zval_unset_isref_p(pz)
#define Z_SET_ISREF_TO_P(pz, isref)   zval_set_isref_to_p(pz, isref)

#define Z_REFCOUNT(z)         Z_REFCOUNT_P(&(z))
#define Z_SET_REFCOUNT(z, rc)     Z_SET_REFCOUNT_P(&(z), rc)
#define Z_ADDREF(z)           Z_ADDREF_P(&(z))
#define Z_DELREF(z)           Z_DELREF_P(&(z))
#define Z_ISREF(z)            Z_ISREF_P(&(z))
#define Z_SET_ISREF(z)          Z_SET_ISREF_P(&(z))
#define Z_UNSET_ISREF(z)        Z_UNSET_ISREF_P(&(z))
#define Z_SET_ISREF_TO(z, isref)    Z_SET_ISREF_TO_P(&(z), isref)

#if defined(__GNUC__)
#define zend_always_inline inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define zend_always_inline __forceinline
#else
#define zend_always_inline inline
#endif

static zend_always_inline zend_uint zval_refcount_p(zval* pz)
{
  return pz->refcount;
}

static zend_always_inline zend_uint zval_set_refcount_p(zval* pz, zend_uint rc)
{
  return pz->refcount = rc;
}

static zend_always_inline zend_uint zval_addref_p(zval* pz)
{
  return ++pz->refcount;
}

static zend_always_inline zend_uint zval_delref_p(zval* pz)
{
  return --pz->refcount;
}

static zend_always_inline zend_bool zval_isref_p(zval* pz)
{
  return pz->is_ref;
}

static zend_always_inline zend_bool zval_set_isref_p(zval* pz)
{
  return pz->is_ref = 1;
}

static zend_always_inline zend_bool zval_unset_isref_p(zval* pz)
{
  return pz->is_ref = 0;
}

static zend_always_inline zend_bool zval_set_isref_to_p(zval* pz, zend_bool isref)
{
  return pz->is_ref = isref;
}

#endif

/* PHP >=5.3 replaced ZVAL_DELREF by Z_DELREF_P */
#if ZEND_MODULE_API_NO >= 20090626
#define ZVAL_DELREF Z_DELREF_P
#define ZVAL_ADDREF Z_ADDREF_P
#endif


#define MAPSCRIPT_REGISTER_CLASS(name, functions, class_entry, constructor) \
    INIT_CLASS_ENTRY(ce, name, functions); \
    class_entry = zend_register_internal_class(&ce TSRMLS_CC); \
    class_entry->create_object = constructor;

#define MAPSCRIPT_ALLOC_OBJECT(zobj, object_type)  \
    zobj = ecalloc(1, sizeof(object_type));

#define MAPSCRIPT_FREE_OBJECT(zobj) \
    zend_hash_destroy(zobj->std.properties); \
    FREE_HASHTABLE(zobj->std.properties);

#define MAPSCRIPT_ADDREF(zobj) if (zobj) Z_ADDREF_P(zobj)

#define MAPSCRIPT_DELREF(zobj) \
    if (zobj) \
    { \
        if (Z_REFCOUNT_P(zobj) == 1) {    \
            zval_ptr_dtor(&zobj);       \
        } \
        else { \
            Z_DELREF_P(zobj);  \
        } \
        zobj = NULL; \
    }


#define MAPSCRIPT_INIT_PARENT(parent) \
    parent.val = NULL; \
    parent.child_ptr = NULL;

#define MAPSCRIPT_FREE_PARENT(parent) \
    if (parent.child_ptr) \
        *parent.child_ptr = NULL; \
    MAPSCRIPT_DELREF(parent.val);

#define MAPSCRIPT_MAKE_PARENT(zobj, ptr)            \
    parent.val = zobj; \
    parent.child_ptr = ptr;

#define MAPSCRIPT_CALL_METHOD_1(zobj, function_name, retval, arg1) \
    zend_call_method_with_1_params(&zobj, Z_OBJCE_P(zobj), NULL, function_name, &retval, arg1);

#define MAPSCRIPT_CALL_METHOD_2(zobj, function_name, retval, arg1, arg2) \
    zend_call_method_with_2_params(&zobj, Z_OBJCE_P(zobj), NULL, function_name, &retval, arg1, arg2);

#define STRING_EQUAL(string1, string2) \
    strcmp(string1, string2) == 0

/* helpers for getters */
#define IF_GET_STRING(property_name, value)  \
    if (strcmp(property, property_name)==0) \
    { \
        RETVAL_STRING( (value ? value:"") , 1);    \
    } \
 
#define IF_GET_LONG(property_name, value)  \
    if (strcmp(property, property_name)==0) \
    { \
        RETVAL_LONG(value); \
    } \
 
#define IF_GET_DOUBLE(property_name, value)  \
    if (strcmp(property, property_name)==0) \
    { \
        RETVAL_DOUBLE(value); \
    } \
 
#define IF_GET_OBJECT(property_name, mapscript_ce, php_object_storage, internal_object) \
    if (strcmp(property, property_name)==0)  \
    {   \
        if (!php_object_storage) {                             \
            mapscript_fetch_object(mapscript_ce, zobj, NULL, (void*)internal_object, \
                                   &php_object_storage TSRMLS_CC); \
        }                                                               \
        RETURN_ZVAL(php_object_storage, 1, 0);                          \
    }

#define CHECK_OBJECT(mapscript_ce, php_object_storage, internal_object) \
    if (!php_object_storage) {                                          \
        mapscript_fetch_object(mapscript_ce, zobj, NULL, (void*)internal_object, \
                           &php_object_storage TSRMLS_CC); \
    }

/* helpers for setters */
#define IF_SET_STRING(property_name, internal, value)        \
    if (strcmp(property, property_name)==0)                  \
    { \
        convert_to_string(value); \
        if (internal) free(internal);    \
        if (Z_STRVAL_P(value))                        \
            internal = strdup(Z_STRVAL_P(value));     \
    }

#define IF_SET_LONG(property_name, internal, value)        \
    if (strcmp(property, property_name)==0)                  \
    { \
        convert_to_long(value); \
        internal = Z_LVAL_P(value);             \
    }

#define IF_SET_DOUBLE(property_name, internal, value)        \
    if (strcmp(property, property_name)==0)                  \
    { \
        convert_to_double(value); \
        internal = Z_DVAL_P(value);             \
    }

#define IF_SET_BYTE(property_name, internal, value)        \
    if (strcmp(property, property_name)==0)                  \
    { \
        convert_to_long(value); \
        internal = (unsigned char)Z_LVAL_P(value);            \
    }

#define IF_SET_COLOR(property_name, internal, value)        \
    if (strcmp(property, property_name)==0)                  \
    { \
        convert_to_long(value); \
        /* validate the color value */ \
        if (Z_LVAL_P(value) < 0 || Z_LVAL_P(value) > 255) {             \
            mapscript_throw_exception("Invalid color value. It must be between 0 and 255." TSRMLS_CC); \
            return;   \
        }             \
        internal = Z_LVAL_P(value);             \
    }


zend_object_value mapscript_object_new(zend_object *zobj,
                                       zend_class_entry *ce,
                                       void (*zend_objects_free_object) TSRMLS_DC);

int mapscript_extract_associative_array(HashTable *php, char **array);

#endif /* PHP_MAPSCRIPT_UTIL_H */

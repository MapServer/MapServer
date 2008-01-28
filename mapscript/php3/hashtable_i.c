/*
 * hashtable_i.c: This file was originally based on the SWIG interface 
 *                file hashtable.i
 *
 * $Id$
 *
 */


#include "php_mapscript.h"



/* ========================================================================
 * Include maphash header, first stating declarations to ignore
 * ======================================================================== */

#include "../../maphash.h"

/* ======================================================================== 
 * Extension methods
 * ======================================================================== */

    // New instance
    hashTableObj *hashTableObj_new() {
        return msCreateHashTable();
    }

    // Destroy instance
    void hashTableObj_destroy(hashTableObj *self) {
        msFreeHashTable(self);
    }

    // set a hash item given key and value
    int hashTableObj_set(hashTableObj *self, const char *key, const char *value) {
        if (msInsertHashTable(self, key, value) == NULL) {
	        return MS_FAILURE;
        }
        return MS_SUCCESS;
    }

    // get value from item by its key
    const char *hashTableObj_get(hashTableObj *self, const char *key) {
        char *value = NULL;
        if (!key) {
            msSetError(MS_HASHERR, "NULL key", "get");
        }
     
        value = (char *) msLookupHashTable(self, key);
        if (!value) {
            msSetError(MS_HASHERR, "Key %s does not exist", "get", key);
            return NULL;
        }
        return value;
    }

    // Remove one item from hash table
    int *hashTableObj_remove(hashTableObj *self, const char *key) {
        return msRemoveHashTable(self, key);
    }

    // Clear all items in hash table (to NULL)
    void *hashTableObj_clear(hashTableObj *self) {
        msFreeHashItems(self);
        initHashTable(self);
    }

    // Return the next key or first key if prevkey == NULL
    char *hashTableObj_nextKey(hashTableObj *self, const char *prevkey) {
        char *key;
        key = msNextKeyFromHashTable(self, prevkey);
        return key;
    }

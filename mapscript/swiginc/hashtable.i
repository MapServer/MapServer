/* $Id$
 */

%{
#include "../../maphash.h"
%}

/* ========================================================================
 * Include maphash header, first stating declarations to ignore
 * ======================================================================== */

// ignore the hashObj struct
%ignore hashObj;

// ignore items and make numitems immutable
%ignore items;
%immutable numitems;

%include "../../maphash.h"

/* ======================================================================== 
 * Extension methods
 * ======================================================================== */

%extend hashTableObj {
    
    // New instance
    hashTableObj(void) {
        return msCreateHashTable();
    }

    // Destroy instance
    ~hashTableObj() {
        msFreeHashTable(self);
    }

    // set a hash item given key and value
    int set(const char *key, const char *value) {
        if (msInsertHashTable(self, key, value) == NULL) {
	        return MS_FAILURE;
        }
        return MS_SUCCESS;
    }

    // get value from item by its key
    char *get(const char *key) {
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
    int *remove(const char *key) {
        return msRemoveHashTable(self, key);
    }

    // Clear all items in hash table (to NULL)
    void *clear(void) {
        msFreeHashItems(self);
        initHashTable(self);
    }

    // Return the next key or first key if prevkey == NULL
    char *nextKey(const char *prevkey=NULL) {
        char *key;
        key = msNextKeyFromHashTable(self, prevkey);
        return key;
    }
    
}




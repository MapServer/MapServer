/* $Id$
 */

%{
#include "../../maphash.h"
%}

/* ========================================================================
 * Include maphash header, first stating declarations to ignore
 * ======================================================================== */

/* ignore the hashObj struct */
%ignore hashObj;

/* ignore items and make numitems immutable */
%ignore items;
%immutable numitems;

%include "../../maphash.h"

/* ======================================================================== 
 * Extension methods
 * ======================================================================== */

%extend hashTableObj {
    
    /* New instance */
#if defined(SWIGJAVA) || defined(SWIGCSHARP)
    hashTableObj() {
#else
    hashTableObj(void) {
#endif
        return msCreateHashTable();
    }

    /* Destroy instance */
    ~hashTableObj() {
        msFreeHashTable(self);
    }

    /* set a hash item given key and value */
    int set(char *key, char *value) {
        if (msInsertHashTable(self, key, value) == NULL) {
	        return MS_FAILURE;
        }
        return MS_SUCCESS;
    }

    /* get value from item by its key */
    char *get(char *key, char *default_value=NULL) {
        char *value = NULL;
        if (!key) {
            msSetError(MS_HASHERR, "NULL key", "get");
        }
     
        value = (char *) msLookupHashTable(self, key);
        if (!value) {
            return default_value;
        }
        return value;
    }

    /* Remove one item from hash table */
    int remove(char *key) {
        return msRemoveHashTable(self, key);
    }

    /* Clear all items in hash table (to NULL) */
    void clear(void) {
        msFreeHashItems(self);
        initHashTable(self);
    }

    /* Return the next key or first key if prevkey == NULL */
    const char *nextKey(char *prevkey=NULL) {
        return msNextKeyFromHashTable(self, (const char *) prevkey);
    }
    
}




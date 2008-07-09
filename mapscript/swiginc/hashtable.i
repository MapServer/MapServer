/* ===========================================================================
   $Id$

   Project:  MapServer
   Purpose:  SWIG interface file for mapscript hashTableObj extensions
   Author:   Sean Gillies, sgillies@frii.com

   ===========================================================================
   Copyright (c) 1996-2001 Regents of the University of Minnesota.

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
   ===========================================================================
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




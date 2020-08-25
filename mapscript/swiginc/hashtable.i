/* ===========================================================================
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

%extend hashTableObj {
    
    /// Create a new instance
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

    /// Set a hash item given key and value. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int set(char *key, char *value) {
        if (msInsertHashTable(self, key, value) == NULL) {
            return MS_FAILURE;
        }
        return MS_SUCCESS;
    }

    /// Returns the value of the item by its key, or default if the key does not exist
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

    /// Removes the hash item by its key. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int remove(char *key) {
        return msRemoveHashTable(self, key);
    }

    /// Empties the table of all items
    void clear(void) {
        msFreeHashItems(self);
        initHashTable(self);
    }

    /// Returns the name of the next key or NULL if there is no valid next key.
    /// If the input key is NULL, returns the first key
    const char *nextKey(char *prevkey=NULL) {
        return msNextKeyFromHashTable(self, (const char *) prevkey);
    }
    
}

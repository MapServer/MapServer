#ifndef MAPHASH__H
#define MAPHASH__H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) && !defined(__CYGWIN__)
#  define MS_DLL_EXPORT     __declspec(dllexport)
#else
#define  MS_DLL_EXPORT
#endif

#define MS_HASHSIZE 41

/* =========================================================================
 * Structs
 * ========================================================================= */

struct hashObj {
    struct hashObj *next;  /* pointer to next item */
    char           *key;   /* string key that is hashed */
    char           *data;  /* string stored in this item */ 
};

typedef struct {
    struct hashObj **items;  /* the hash table */
    int              numitems;  /* number of items */
} hashTableObj;

/* =========================================================================
 * Functions
 * ========================================================================= */

/* msCreateHashTable - create a hash table
 * ARGS:
 *     None
 * RETURNS:
 *     New hash table
 */
MS_DLL_EXPORT hashTableObj *msCreateHashTable( void );

/* For use in mapfile.c with hashTableObj structure members */
MS_DLL_EXPORT int initHashTable( hashTableObj *table );

/* msFreeHashTable - free allocated memory
 * ARGS:
 *     table - target hash table
 * RETURNS:
 *     None
 */
MS_DLL_EXPORT void msFreeHashTable( hashTableObj *table );

/* Free only the items for hashTableObj structure members (metadata, &c) */
MS_DLL_EXPORT void msFreeHashItems( hashTableObj *table );

/* msInsertHashTable - insert new item
 * ARGS:
 *     table - the target hash table
 *     key   - key string for new item
 *     value - data string for new item
 * RETURNS:
 *     pointer to the new item or NULL
 * EXCEPTIONS:
 *     raise MS_HASHERR on failure
 */
MS_DLL_EXPORT struct hashObj *msInsertHashTable( hashTableObj *table,
                                                 const char *key,
                                                 const char *value );

/* msLookupHashTable - get the value of an item by its key
 * ARGS:
 *     table - the target hash table
 *     key   - key string of item
 * RETURNS:
 *     string value of item
 */
MS_DLL_EXPORT char *msLookupHashTable( hashTableObj *table, const char *key);

/* msRemoveHashTable - remove item from table at key
 * ARGS:
 *     table - target hash table
 *     key   - key string
 * RETURNS:
 *     MS_SUCCESS or MS_FAILURE
 */
MS_DLL_EXPORT int msRemoveHashTable( hashTableObj *table, const char *key);

/* msFirstKeyFromHashTable - get key of first item
 * ARGS:
 *     table - target hash table
 * RETURNS:
 *     first key as a string
 */
MS_DLL_EXPORT const char *msFirstKeyFromHashTable( hashTableObj *table );

/* msNextKeyFromHashTable - get next key
 * ARGS:
 *     table - target hash table
 *     prevkey - the previous key
 * RETURNS:
 *     the key of the item of following prevkey as a string
 */
MS_DLL_EXPORT const char *msNextKeyFromHashTable( hashTableObj *table,
                                                  const char *prevkey );

#ifdef __cplusplus
}
#endif

#endif /* MAPHASH__H */

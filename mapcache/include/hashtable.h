/**
 * Hashtable implementation.  Most of the code is from Algorithms With C (Loudon) 
 */

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "list.h"

typedef struct hashtable_tag    {
    int     buckets;
    int     (*h)(const void* key);
    int     (*match)(const void* key1, const void* key2);
    void    (*destroy)(void *data);
    
    int     size;
    
    list*   table;
} hashtable;

typedef struct hashtable_iter_tag hashtable_iter;

/**
 * Initializes the given hashtable, with the given number of buckets to hold values.  Takes
 * pointers to functions for:
 *   h: The hash function.  If NULL, P.J. Weinberger's string hash function from the Dragon Book
 *      will be used
 *   match: The match function.  Used to determine if two keys match.  If NULL, !strcmp will be 
 *      used
 *   destroy: The function to call when the value data of a key needs to be cleaned-up.  Pass in 0
 *      if you wish to take care of this yourself, or you could pass in free() if the objects are simple
 *
 * Returns 0 if the hashtable was initialized successfully, -1 otherwise    
 */
int ht_init(hashtable* ht, int buckets, int (*h)(const void* key), 
    int (*match)(const void* key1, const void* key2), 
    void (*destroy)(void *data));
    
/** 
 * Destroys the given hashtable, calling the user-supplied "destroy" function on each value in the hash
 */
void ht_destroy(hashtable* ht);

/**
 * Inserts a new value in the given hashtable
 *
 * Returns 0 if instering the element was successful, 1 if the element was already in the table, 
 * -1 if there was a problem
 */
int ht_insert(hashtable* ht, const void* data);

/**
 * Removes an element from the given hashtable.  If successful, data contains a pointer to the data 
 * removed.  It is up to the caller to further manage this data.
 *
 * 0 if removing the element was successful, -1 otherwise
 */
int ht_remove(hashtable* ht, void** data);

/**
 * Determines whether an element matches the given data in the hashtable.  If so, data points to 
 * the matched value in the hashtable
 *
 * Returns 0 if a match was found in the hashtable, -1 otherwise
 */
int ht_lookup(hashtable* ht, void** data);

/**
 * Begin iterating through the given hashtable
 *
 * Returns an iterator representing the first item in the table, or NULL if the table is empty
 */
hashtable_iter* ht_iter_begin(hashtable* ht);

/**
 * Given the current iterator, return an iterator representing the next item in the hashtable
 * NOTE: Do NOT assume that the iterator passed in to "current" is still valid after this call!
 *       Also, items in the hashtable are NOT guaranteed to be iterated in any particular order
 *
 * Returns NULL if we have reached the end of the list
 */
hashtable_iter* ht_iter_next(hashtable_iter* current);

/**
 * Returns the value of the given iterator
 */
void* ht_value(hashtable_iter* iter);

/**
 * The default hashing function - works great for strings.  This was taken from the venerable
 * Dragon Book and Algorithms With C and was created by P.J. Weinberger
 *
 */
int ht_hashpjw(const void* key);

/**
 * Returns the number of keys in the hashtable
 */
#define ht_size(hashtable) ((hashtable)->size)

#endif // HASHTABLE_H
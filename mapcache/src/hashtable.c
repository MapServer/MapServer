/**
 * Hashtable implementation
 */

#include <stdlib.h>
#include <string.h>

#include "hashtable.h"

typedef struct hashtable_iter_tag   {
    hashtable* ht;

    int current_bucket;
    list_element* current;
} hashtable_iter_impl;

// Prototypes for some default functions if none other are given
int hashpjw(const void* key);
int matchstr(const void* key1, const void* key2);

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
    void (*destroy)(void *data))    {
    
    int i;  
    
    if ((ht->table = (list*)malloc(buckets * sizeof(list))) == 0)   {
        return -1;
    }
    
    ht->buckets = buckets;
    for (i = 0; i < ht->buckets; i++)   {
        list_init(&ht->table[i], destroy);
    }

    ht->h       = h? h : ht_hashpjw;
    ht->match   = match? match : matchstr;
    ht->destroy = destroy;
    ht->size = 0;
    
    return 0;
}
    
/** 
 * Destroys the given hashtable, calling the user-supplied "destroy" function on each value in the hash
 */
void ht_destroy(hashtable* ht)  {
    int i;
    
    for (i = 0; i < ht->buckets; i++)   {
        list_destroy(&ht->table[i]);
    }
    
    free(ht->table);
    memset(ht, 0, sizeof(hashtable));
}

/**
 * Inserts a new value in the given hashtable
 *
 * Returns 0 if instering the element was successful, 1 if the element was already in the table, 
 * -1 if there was a problem
 */
int ht_insert(hashtable* ht, const void* data)  {
    void* temp;
    int bucket, retval;
    
    temp = (void*)data;
    if (ht_lookup(ht, &temp) == 0)  {
        // Do nothing, return 1 to signify that the element was already in the table
        return 1;
    }
    
    // Insert the data into the proper bucket
    bucket = ht->h(data) % ht->buckets;
    if ((retval = list_insert_next(&ht->table[bucket], NULL, data)) == 0)   {
        ht->size++;
    }
    
    return retval;
}

/**
 * Removes an element from the given hashtable.  If successful, data contains a pointer to the data 
 * removed.  It is up to the caller to further manage this data.
 *
 * 0 if removing the element was successful, -1 otherwise
 */
int ht_remove(hashtable* ht, void** data)   {
    list_element *element, *prev;
    
    int bucket;
    
    // Hash the key, then search for the data in the given bucket
    bucket = ht->h(*data) % ht->buckets;
    prev = 0;
    for (element = list_head(&ht->table[bucket]); element != 0; element = list_next(element))   {
        if (ht->match(*data, list_data(element)))   {
            // Found it, remove it
            if (list_remove_next(&ht->table[bucket], prev, data) == 0)  {
                ht->size--;
                return 0;
            }
        }
        
        prev = element;
    }
    
    // Data not found
    return -1;
}

/**
 * Determines whether an element matches the given data in the hashtable.  If so, data points to 
 * the matched value in the hashtable
 *
 * Returns 0 if a match was found in the hashtable, -1 otherwise
 */
int ht_lookup(hashtable* ht, void** data)   {
    list_element *element;
    int bucket;
    
    // Hash the key, then search for the data in the bucket
    bucket = ht->h(*data) % ht->buckets;
    for (element = list_head(&ht->table[bucket]); element != 0; element = list_next(element))   {
        if (ht->match(*data, list_data(element)))   {
            // We found it
            *data = list_data(element);
            return 0;
        }
    }
    
    // Data not found
    return -1;
}

/**
 * Begin iterating through the hashtable
 *
 * Returns an iterator representing the first item in the table, or NULL if the table is empty
 */
hashtable_iter* ht_iter_begin(hashtable* ht)    {
    hashtable_iter_impl* hi;
    list_element* elem;
    int i;
    
    hi = 0;
    elem = 0;
    for (i = 0; i < ht->buckets; i++)   {
        elem = list_head(&ht->table[i]);
        if (elem)   {
            break;
        }
    }
    
    if (elem)   {
        hi = (hashtable_iter_impl*)malloc(sizeof(hashtable_iter_impl));
        hi->ht = ht;
        hi->current_bucket = i;
        hi->current = elem;
    }
    
    return hi;
}

/**
 * Given the current iterator, return an iterator representing the next item in the hashtable
 * NOTE: Do NOT assume that the iterator passed in to "current" is still valid after this call!
 *       Also, items in the hashtable are NOT guaranteed to be iterated in any particular order
 *
 * Returns NULL if we have reached the end of the list
 */
hashtable_iter* ht_iter_next(hashtable_iter* current)   {
    hashtable_iter_impl* cur;
    list_element* elem;
    int i;
    
    if (!current)   {
        return 0;
    }
    
    cur = (hashtable_iter_impl*)current;
    i = cur->current_bucket;
    elem = cur->current->next;
    if (!elem)  {
        for (i = cur->current_bucket+1; i < cur->ht->buckets; i++)  {
            elem = list_head(&cur->ht->table[i]);
            if (elem)   {
                break;
            }
        }
    }
    
    if (elem)   {
        cur->current_bucket = i;
        cur->current = elem; 
    } else {
        free(cur);
        cur = 0;
    }
    
    return cur;
}

/**
 * Returns the value of the given iterator
 */
void* ht_value(hashtable_iter* iter)    {
    return ((hashtable_iter_impl*)iter)->current->data;
}

/**
 * The default hashing function - works great for strings.  This was taken from the venerable
 * Dragon Book and Algorithms With C and was created by P.J. Weinberger
 *
 */
int ht_hashpjw(const void* key) {
    const char* ptr;
    unsigned int val;
    
    // Hash the key by performing some bit manipulation on it
    val = 0;
    ptr = key;
    
    while (*ptr != '\0')    {
        unsigned int tmp;
        
        val = (val << 4) + (*ptr);
        
        if (tmp = (val & 0xf0000000))   {
            val = val ^ (tmp >> 24);
            val = val ^ tmp;
        }
        
        ptr++;
    }
    
    return val;
}

/**
 * The default compare function.  Just a strcmp
 */ 
int matchstr(const void* key1, const void* key2)    {
    return !strcmp((const char*)key1, (const char*)key2);
}
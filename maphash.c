#include <ctype.h>

#include "map.h"
#include "maphash.h"

static unsigned hash(const char *key)
{
  unsigned hashval;
  
  for(hashval=0; *key!='\0'; key++)
    hashval = tolower(*key) + 31 * hashval;

  return(hashval % MS_HASHSIZE);
}

hashTableObj *msCreateHashTable() 
{
    int i;
    hashTableObj *table;
    
    table = (hashTableObj *) malloc(sizeof(hashTableObj));
    table->items = (struct hashObj **) malloc(sizeof(struct hashObj *)*MS_HASHSIZE);
    
    for (i=0; i<MS_HASHSIZE; i++)
        table->items[i] = NULL;
    table->numitems = 0;
    
    return table;
}

int initHashTable( hashTableObj *table ) 
{
    int i;

    table->items = (struct hashObj **) malloc(sizeof(struct hashObj *)*MS_HASHSIZE);
    if (!table->items) {
        msSetError(MS_MEMERR, "Failed to allocate memory for items", "initHashTable");
        return MS_FAILURE;
    }
    for (i=0; i<MS_HASHSIZE; i++)
        table->items[i] = NULL;
    table->numitems = 0;
    return MS_SUCCESS;
}

void msFreeHashTable( hashTableObj *table )
{
    int i;
    struct hashObj *tp=NULL;
    struct hashObj *prev_tp=NULL;

    if (!table) return;

    for (i=0; i<MS_HASHSIZE; i++) {
        if (table->items[i] != NULL) {
            for (tp=table->items[i]; tp!=NULL; prev_tp=tp,tp=tp->next,free(prev_tp)) {
	            free(tp->key);
	            free(tp->data);
            }
        }
        if (tp) free(tp);
    }
    free(table->items);
    free(table);
    table = NULL;
}

void msFreeHashItems( hashTableObj *table )
{
    int i;
    struct hashObj *tp=NULL;
    struct hashObj *prev_tp=NULL;

    if (!table) return;

    for (i=0; i<MS_HASHSIZE; i++) {
        if (table->items[i] != NULL) {
            for (tp=table->items[i]; tp!=NULL; prev_tp=tp,tp=tp->next,free(prev_tp)) {
	            free(tp->key);
	            free(tp->data);
            }
        }
        if (tp) free(tp);
    }
    free(table->items);
}

struct hashObj *msInsertHashTable(hashTableObj *table, 
                                  const char *key, const char *value)
{
    struct hashObj *tp;
    unsigned hashval;

    if (!table || !key || !value) {
        msSetError(MS_HASHERR, "Invalid hash table or key",
                               "msInsertHashTable");
        return NULL;
    }

    for (tp=table->items[hash(key)]; tp!=NULL; tp=tp->next)
        if(strcasecmp(key, tp->key) == 0)
            break;

    if (tp == NULL) { /* not found */
        tp = (struct hashObj *) malloc(sizeof(*tp));
        if ((tp == NULL) || (tp->key = strdup(key)) == NULL) {
            msSetError(MS_HASHERR, "No such hash entry", "msInsertHashTable");
            return NULL;
        }
        hashval = hash(key);
        tp->next = table->items[hashval];
        table->items[hashval] = tp;
        table->numitems++;
    } else {
        free(tp->data);
    }

    if ((tp->data = strdup(value)) == NULL)
        return NULL;

    return tp;
}

char *msLookupHashTable(hashTableObj *table, const char *key)
{
    struct hashObj *tp;

    if (!table || !key) {
        return(NULL);
    }

    for (tp=table->items[hash(key)]; tp!=NULL; tp=tp->next)
        if (strcasecmp(key, tp->key) == 0)
            return(tp->data);

    return NULL;
}

int msRemoveHashTable(hashTableObj *table, const char *key)
{ 
    struct hashObj *tp;
    struct hashObj *prev_tp=NULL;
    int success = 0;

    if (!table || !key) {
        msSetError(MS_HASHERR, "No hash table", "msRemoveHashTable");
        return MS_FAILURE;
    }
  
    tp=table->items[hash(key)];
    if (!tp) {
        msSetError(MS_HASHERR, "No such hash entry", "msRemoveHashTable");
        return MS_FAILURE;
    }

    prev_tp = NULL;
    while (tp != NULL) {
        if (strcasecmp(key, tp->key) == 0) {
            if (prev_tp) {     
                prev_tp->next = tp->next;
                free(tp);
                break;
            }
            else {
                table->items[hash(key)] = NULL;
                free(tp);
                break;
            }
            success =1;
        }
        prev_tp = tp;
        tp = tp->next;
    }

    if (success) {
        table->numitems--;
        return MS_SUCCESS;
    }

    return MS_FAILURE;
}
      
const char *msFirstKeyFromHashTable( hashTableObj *table )
{
    int hash_index;
    
    if (!table) {
        msSetError(MS_HASHERR, "No hash table", "msFirstKeyFromHashTable");
        return NULL;
    }
    
    for (hash_index = 0; hash_index < MS_HASHSIZE; hash_index++ ) {
        if (table->items[hash_index] != NULL )
            return table->items[hash_index]->key;
    }

    return NULL;
}

const char *msNextKeyFromHashTable( hashTableObj *table, const char *lastKey )
{
    int hash_index;
    struct hashObj *link;

    if (!table) {
        msSetError(MS_HASHERR, "No hash table", "msNextKeyFromHashTable");
        return NULL;
    }
    
    if ( lastKey == NULL )
        return msFirstKeyFromHashTable( table );

    hash_index = hash(lastKey);
    for ( link = table->items[hash_index]; 
         link != NULL && strcasecmp(lastKey,link->key) != 0;
         link = link->next ) {}
    
    if ( link != NULL && link->next != NULL )
        return link->next->key;

    while ( ++hash_index < MS_HASHSIZE ) {
        if ( table->items[hash_index] != NULL )
            return table->items[hash_index]->key;
    }

    return NULL;
}


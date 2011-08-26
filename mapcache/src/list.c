/**
 * Linked List implementation
 */ 

#include <stdlib.h>
#include <string.h>

#include "list.h"

/**
 * Initializes the list pointed to by "l".  destory is a pointer to a function that is called
 * when an element from the list is freed.  This can be null if no memory management needs to take place,
 * or it could simply be free() if the data is simple
 */
void list_init(list* l, void (*destroy)(void* data))    {
    l->size = 0;
    l->destroy = destroy;
    
    l->head = 0;
    l->tail = 0;
}

/**
 * Destroys the entire list pointed to be "l"
 */
void list_destroy(list* l)  {
    void* data;
    
    while (list_size(l) > 0)    {
        if (list_remove_next(l, NULL, (void**)&data) == 0 && l->destroy)    {
            l->destroy(data);
        }
    }
    
    // Zero out the structure so no junk data
    memset(l, 0, sizeof(list));
}

/**
 * Inserts data into list "l" in the node after "after".  If "after" is NULL, the data is inserted
 * as the head.
 *
 * Returns 0 if successful, -1 otherwise
 */
int list_insert_next(list* l, list_element* after, const void* data)    {
    list_element* new;
    
    new = (list_element*)malloc(sizeof(list_element));
    new->data = (void*)data;
    
    if (after == 0) {
        if (list_size(l) == 0)  {
            l->tail = new;
        }
        
        new->next = l->head;
        l->head = new;
    } else {
        if (after->next == 0)   {
            l->tail = new;
        }
        
        new->next = after->next;
        after->next = new;
    }
    
    l->size++;
    return 0;
}

/**
 * Removes the element after the given element "after" from list "l".  "data" will contain a pointer
 * to the data managed by the removed element. list->destroy will NOT be called on this data, so it 
 * is up to the caller to clean up the data item
 *
 * Returns 0 if successful, -1 otherwise
 */
int list_remove_next(list *l, list_element* after, void **data) {
    list_element *old;
    
    if (list_size(l) == 0)  {
        return -1;
    }
    
    if (after == 0) {
        *data = l->head->data;
        old = l->head;
        l->head = l->head->next;
        
        if (list_size(l) == 1)  {
            l->tail = NULL;
        }
    } else {
        
        if (after->next == 0)   {
            return -1;
        }
        
        *data = after->next->data;
        old = after->next;
        after->next = after->next->next;
        
        if (after->next == NULL)    {
            l->tail = after;
        }
    }
    
    free(old);
    
    l->size--;
    return 0;
}
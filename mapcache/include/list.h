/**
 * List implementation.  Most of the code is from Algorithms With C (Loudon) 
 */
#ifndef LIST_H
#define LIST_H

typedef struct list_element_tag {
    void*                       data;
    struct list_element_tag*    next;
} list_element;

typedef struct list_tag {
    int size;
    
    void (*destroy)(void *data);            
    
    list_element* head;
    list_element* tail;
} list;

/**
 * Initializes the list pointed to by "l".  destory is a pointer to a function that is called
 * when an element from the list is freed.  This can be null if no memory management needs to take place,
 * or it could simply be free() if the data is simple
 */
void list_init(list* l, void (*destroy)(void* data));

/**
 * Destroys the entire list pointed to be "l"
 */
void list_destroy(list* l);

/**
 * Inserts data into list "l" in the node after "after".  If "after" is NULL, the data is inserted
 * as the head.
 *
 * Returns 0 if successful, -1 otherwise
 */
int list_insert_next(list* l, list_element* after, const void* data);

/**
 * Removes the element after the given element "after" from list "l".  "data" will contain a pointer
 * to the data managed by the removed element. list->destroy will NOT be called on this data, so it 
 * is up to the caller to clean up the data item
 *
 * Returns 0 if successful, -1 otherwise
 */
int list_remove_next(list *l, list_element* after, void **data);

/**
 * Returns the number of elements in the list
 */
#define list_size(list) ((list)->size)

/**
 * Returns the head element of a list
 */ 
#define list_head(list) ((list)->head)

/**
 * Returns the tail (last) element of a list
 */
#define list_tail(list) ((list)->tail)

/**
 * Returns 1 if the given element is at the head of the given list, 0 otherwise
 */
#define list_is_head(list, element) ((element) == (list)->head? 1 : 0)

/**
 * Returns 1 if the given element is at the tail of the given list, 0 otherwise
 */
#define list_is_tail(list, element) ((element) == (list)->tail? 1 : 0)

/**
 * Returns the data contained in the given element
 */
#define list_data(element) ((element)->data)

/**
 * Returns the next element after the given element
 */
#define list_next(element) ((element)->next)

#endif // LIST_H
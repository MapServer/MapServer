#ifndef MAPHASH__H
#define MAPHASH__H

#define MS_HASHSIZE 40

struct hashObj { /* hash table entry */
  struct hashObj *next;
  char *key;
  char *data;
};

typedef struct hashObj **  hashTableObj;

hashTableObj msCreateHashTable();
struct hashObj *msInsertHashTable(hashTableObj, char *, char *);
char *msLookupHashTable(hashTableObj, char *);
void msFreeHashTable(hashTableObj);

#endif /* MAPHASH__H */

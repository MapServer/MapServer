#ifndef MAPHASH__H
#define MAPHASH__H

#ifdef __cplusplus
extern "C" {
#endif

#define MS_HASHSIZE 41

struct hashObj { /* hash table entry */
  struct hashObj *next;
  char *key;
  char *data;
};

typedef struct hashObj **  hashTableObj;

hashTableObj msCreateHashTable(void);
struct hashObj *msInsertHashTable(hashTableObj, const char *, const char *);
char *msLookupHashTable(hashTableObj, const char *);
void msFreeHashTable(hashTableObj);
int msRemoveHashTable(hashTableObj table, const char *string);
const char *msFirstKeyFromHashTable( hashTableObj table );
const char *msNextKeyFromHashTable( hashTableObj table, const char *lastKey );

#ifdef __cplusplus
}
#endif

#endif /* MAPHASH__H */

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

struct hashObj { /* hash table entry */
  struct hashObj *next;
  char *key;
  char *data;
};

typedef struct hashObj **  hashTableObj;

MS_DLL_EXPORT hashTableObj msCreateHashTable(void);
MS_DLL_EXPORT struct hashObj *msInsertHashTable(hashTableObj, const char *, const char *);
MS_DLL_EXPORT char *msLookupHashTable(hashTableObj, const char *);
MS_DLL_EXPORT void msFreeHashTable(hashTableObj);
MS_DLL_EXPORT int msRemoveHashTable(hashTableObj table, const char *string);
MS_DLL_EXPORT const char *msFirstKeyFromHashTable( hashTableObj table );
MS_DLL_EXPORT const char *msNextKeyFromHashTable( hashTableObj table, const char *lastKey );

#ifdef __cplusplus
}
#endif

#endif /* MAPHASH__H */
